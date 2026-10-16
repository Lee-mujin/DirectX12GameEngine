#include "pch.h"
#include "ModelLoader.h"
#include "Mesh.h"
#include "Vertex.h"
#include "SkinnedMesh.h"
#include "Skeleton.h"
#include "Animation.h"
#include "UploadContext.h"
#include "Material.h"
#include "TextureLoader.h"
#include "Texture.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <unordered_map>
#include <algorithm>
#include <filesystem>

namespace
{
    Matrix4x4 AssimpMatrixToOurMatrix(const aiMatrix4x4& m)
    {
        return Matrix4x4(
            m.a1, m.b1, m.c1, m.d1,
            m.a2, m.b2, m.c2, m.d2,
            m.a3, m.b3, m.c3, m.d3,
            m.a4, m.b4, m.c4, m.d4
        );
    }

    std::string ResolveValidPath(const std::string& inputPath)
    {
        namespace fs = std::filesystem;

        if (fs::exists(inputPath) && !fs::is_directory(inputPath))
        {
            return fs::absolute(inputPath).string();
        }

        std::string fallbackPath = "Assets/" + inputPath;
        if (fs::exists(fallbackPath) && !fs::is_directory(fallbackPath))
        {
            return fs::absolute(fallbackPath).string();
        }

        std::string fileName = fs::path(inputPath).filename().string();
        std::string fallbackName = "Assets/" + fileName;
        if (fs::exists(fallbackName) && !fs::is_directory(fallbackName))
        {
            return fs::absolute(fallbackName).string();
        }
        return "";
    }

    //메시 지오메트리(Position, Normal, Tangent, UV) 및 서브메시 추출 함수
    void ExtractGeometry(
        const aiScene* scene,
        std::vector<Vertex>& outVertices,
        std::vector<UINT32>& outIndices,
        std::vector<UINT32>& outMeshOffsets,
        std::vector<SubMesh>& outSubMeshes)
    {
        UINT32 currentOffset = 0;
        outMeshOffsets.resize(scene->mNumMeshes);
        outSubMeshes.clear();
        outSubMeshes.reserve(scene->mNumMeshes);

        for (unsigned int m = 0; m < scene->mNumMeshes; ++m)
        {
            const aiMesh* mesh = scene->mMeshes[m];
            outMeshOffsets[m] = currentOffset;

            SubMesh subMesh;
            subMesh.IndexStart = static_cast<UINT>(outIndices.size());
            subMesh.BaseVertexLocation = static_cast<INT>(currentOffset);
            subMesh.MaterialIndex = mesh->mMaterialIndex;

            for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
            {
                Vertex v = {};
                v.position[0] = mesh->mVertices[i].x;
                v.position[1] = mesh->mVertices[i].y;
                v.position[2] = mesh->mVertices[i].z;

                subMesh.Bounds.Encapsulate(Vector3(v.position[0], v.position[1], v.position[2]));

                if (mesh->HasNormals())
                {
                    v.normal[0] = mesh->mNormals[i].x;
                    v.normal[1] = mesh->mNormals[i].y;
                    v.normal[2] = mesh->mNormals[i].z;
                }

                if (mesh->HasTangentsAndBitangents())
                {
                    v.tangent[0] = mesh->mTangents[i].x;
                    v.tangent[1] = mesh->mTangents[i].y;
                    v.tangent[2] = mesh->mTangents[i].z;

                    if (mesh->HasNormals())
                    {
                        aiVector3D n = mesh->mNormals[i];
                        aiVector3D t = mesh->mTangents[i];
                        aiVector3D b = mesh->mBitangents[i];
                        aiVector3D cross = n ^ t;
                        v.tangent[3] = ((cross * b) < 0.0f) ? -1.0f : 1.0f;
                    }
                    else
                    {
                        v.tangent[3] = 1.0f;
                    }
                }
                else
                {
                    v.tangent[0] = 1.0f;
                    v.tangent[3] = 1.0f;
                }

                if (mesh->HasTextureCoords(0))
                {
                    v.uv[0] = mesh->mTextureCoords[0][i].x;
                    v.uv[1] = mesh->mTextureCoords[0][i].y;
                }

                v.color[0] = v.color[1] = v.color[2] = v.color[3] = 1.0f;
                outVertices.push_back(v);
            }

            //로컬 인덱스(0부터 시작) 그대로 저장 (DirectX 12 BaseVertexLocation 활용)
            for (unsigned int f = 0; f < mesh->mNumFaces; ++f)
            {
                const aiFace& face = mesh->mFaces[f];
                if (face.mNumIndices == 3)
                {
                    outIndices.push_back(face.mIndices[0]);
                    outIndices.push_back(face.mIndices[1]);
                    outIndices.push_back(face.mIndices[2]);
                }
            }

            subMesh.IndexCount = static_cast<UINT>(outIndices.size()) - subMesh.IndexStart;
            outSubMeshes.push_back(subMesh);

            currentOffset += mesh->mNumVertices;
        }
    }

    BoundingBox CalculateBounds(const std::vector<Vertex>& vertices)
    {
        BoundingBox box;
        for (const auto& v : vertices)
        {
            box.Encapsulate(Vector3(v.position[0], v.position[1], v.position[2]));
        }
        return box;
    }
}

//LoadMaterials
std::vector<std::shared_ptr<Material>> ModelLoader::LoadMaterials(
    const aiScene* scene,
    const std::string& modelPath,
    TextureLoader* textureLoader)
{
    if (!scene || scene->mNumMaterials == 0)
    {
        return {};
    }

    std::vector<std::shared_ptr<Material>> materials;
    materials.reserve(scene->mNumMaterials);

    std::unordered_map<std::string, std::shared_ptr<Texture>> textureCache;

    auto LoadTextureFromAssimp = [&](const aiString& texPath) -> std::shared_ptr<Texture>
    {
        if (texPath.length == 0 || !textureLoader) return nullptr;
        std::string pathStr = texPath.C_Str();

        auto it = textureCache.find(pathStr);
        if (it != textureCache.end()) return it->second;

        std::shared_ptr<Texture> loadedTexture = nullptr;

        // 1) 임베디드 텍스처 (*0, *1 등 또는 scene->GetEmbeddedTexture)
        const aiTexture* embeddedTex = scene->GetEmbeddedTexture(pathStr.c_str());
        if (embeddedTex)
        {
            if (embeddedTex->mHeight == 0)
            {
                // 압축 포맷 (PNG, JPG 등): mWidth가 파일 바이트 수
                loadedTexture = textureLoader->LoadTextureFromMemory(embeddedTex->pcData, embeddedTex->mWidth);
            }
            else
            {
                // 비압축 ARGB8888/RGBA 포맷: aiTexel (b, g, r, a) -> RGBA 변환
                std::vector<BYTE> rgba(embeddedTex->mWidth * embeddedTex->mHeight * 4);
                const aiTexel* src = embeddedTex->pcData;
                for (size_t p = 0; p < embeddedTex->mWidth * embeddedTex->mHeight; ++p)
                {
                    rgba[p * 4 + 0] = src[p].r;
                    rgba[p * 4 + 1] = src[p].g;
                    rgba[p * 4 + 2] = src[p].b;
                    rgba[p * 4 + 3] = src[p].a;
                }
                loadedTexture = textureLoader->CreateTextureFromRawRGBA(rgba.data(), embeddedTex->mWidth, embeddedTex->mHeight);
            }
        }
        else
        {
            // 2) 외부 파일 경로: 모델 경로 기준 상대 경로 해석
            namespace fs = std::filesystem;
            fs::path modelDir = fs::path(modelPath).parent_path();
            fs::path resolvedPath = modelDir / pathStr;

            if (!fs::exists(resolvedPath))
            {
                resolvedPath = modelDir / fs::path(pathStr).filename();
            }

            if (fs::exists(resolvedPath))
            {
                loadedTexture = textureLoader->LoadTextureFromFile(resolvedPath.wstring());
            }
        }

        if (loadedTexture)
        {
            textureCache[pathStr] = loadedTexture;
        }
        return loadedTexture;
    };

    for (unsigned int i = 0; i < scene->mNumMaterials; ++i)
    {
        const aiMaterial* aiMat = scene->mMaterials[i];
        auto mat = std::make_shared<Material>();

        // 1. Base Color (Albedo) Factor
        aiColor4D baseColor(1.0f, 1.0f, 1.0f, 1.0f);
        if (aiMat->Get(AI_MATKEY_BASE_COLOR, baseColor) == AI_SUCCESS ||
            aiMat->Get(AI_MATKEY_COLOR_DIFFUSE, baseColor) == AI_SUCCESS)
        {
            mat->SetBaseColor(Vector3(baseColor.r, baseColor.g, baseColor.b));
        }

        // 2. Metallic Factor
        float metallic = 0.0f;
        if (aiMat->Get(AI_MATKEY_METALLIC_FACTOR, metallic) == AI_SUCCESS)
        {
            mat->SetMetallic(std::clamp(metallic, 0.0f, 1.0f));
        }

        // 3. Roughness Factor
        float roughness = 0.5f;
        if (aiMat->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) == AI_SUCCESS)
        {
            mat->SetRoughness(std::clamp(roughness, 0.04f, 1.0f));
        }

        // 4. Base Color Texture (glTF BASE_COLOR or OBJ/FBX DIFFUSE)
        aiString texPath;
        if (aiMat->GetTexture(aiTextureType_BASE_COLOR, 0, &texPath) == AI_SUCCESS ||
            aiMat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS)
        {
            auto tex = LoadTextureFromAssimp(texPath);
            if (tex)
            {
                mat->SetBaseColorTexture(tex);
            }
        }

        // 5. Normal Texture (glTF NORMALS or OBJ/FBX HEIGHT)
        aiString normalPath;
        if (aiMat->GetTexture(aiTextureType_NORMALS, 0, &normalPath) == AI_SUCCESS ||
            aiMat->GetTexture(aiTextureType_HEIGHT, 0, &normalPath) == AI_SUCCESS)
        {
            auto normTex = LoadTextureFromAssimp(normalPath);
            if (normTex)
            {
                mat->SetNormalTexture(normTex);
            }
        }

        materials.push_back(mat);
    }

    return materials;
}

//LoadStaticMesh
std::shared_ptr<Mesh> ModelLoader::LoadStaticMesh(
    ID3D12Device* device,
    UploadContext* uploadContext,
    const std::string& path,
    std::vector<std::shared_ptr<Material>>* outMaterials,
    TextureLoader* textureLoader)
{
    if (outMaterials) outMaterials->clear();
    std::string validPath = ResolveValidPath(path);
    if (validPath.empty()) return nullptr;

    Assimp::Importer importer;
    unsigned int flags =
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_CalcTangentSpace |
        aiProcess_JoinIdenticalVertices |
        aiProcess_ConvertToLeftHanded;

    const aiScene* scene = importer.ReadFile(validPath, flags);
    if (!scene || !scene->mRootNode || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE))
    {
        OutputDebugStringA("[ModelLoader] Failed to load static mesh: ");
        OutputDebugStringA(importer.GetErrorString());
        OutputDebugStringA("\n");
        return nullptr;
    }

    if (outMaterials)
    {
        *outMaterials = LoadMaterials(scene, validPath, textureLoader);
    }

    std::vector<Vertex> vertices;
    std::vector<UINT32> indices;
    std::vector<UINT32> meshOffsets;
    std::vector<SubMesh> subMeshes;
    ExtractGeometry(scene, vertices, indices, meshOffsets, subMeshes);

    if (vertices.empty() || indices.empty()) return nullptr;

    auto result = std::make_shared<Mesh>();
    result->Create(device, uploadContext, vertices, indices);
    result->SetSubMeshes(std::move(subMeshes));
    result->SetBoundingBox(CalculateBounds(vertices));

    return result;
}

//LoadSkinnedMesh
std::shared_ptr<SkinnedMesh> ModelLoader::LoadSkinnedMesh(
    ID3D12Device* device,
    UploadContext* uploadContext,
    const std::string& path,
    std::vector<std::shared_ptr<Animation>>* outAnimations,
    std::vector<std::shared_ptr<Material>>* outMaterials,
    TextureLoader* textureLoader)
{
    if (outAnimations) outAnimations->clear();
    if (outMaterials) outMaterials->clear();

    std::string validPath = ResolveValidPath(path);
    if (validPath.empty()) return nullptr;

    Assimp::Importer importer;
    unsigned int flags =
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_CalcTangentSpace |
        aiProcess_LimitBoneWeights |
        aiProcess_ConvertToLeftHanded;

    const aiScene* scene = importer.ReadFile(validPath, flags);
    if (!scene || !scene->mRootNode || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE))
    {
        OutputDebugStringA("[ModelLoader] Failed to load skinned mesh: ");
        OutputDebugStringA(importer.GetErrorString());
        OutputDebugStringA("\n");
        return nullptr;
    }

    if (outMaterials)
    {
        *outMaterials = LoadMaterials(scene, validPath, textureLoader);
    }

    //버텍스에 영향을 주는 실제 본 개수 확인
    unsigned int totalBoneCount = 0;
    for (unsigned int m = 0; m < scene->mNumMeshes; ++m)
    {
        totalBoneCount += scene->mMeshes[m]->mNumBones;
    }

    if (totalBoneCount == 0)
    {
        return nullptr;
    }

    //공통 지오메트리 추출
    std::vector<Vertex> vertices;
    std::vector<UINT32> indices;
    std::vector<UINT32> meshOffsets;
    std::vector<SubMesh> subMeshes;
    ExtractGeometry(scene, vertices, indices, meshOffsets, subMeshes);

    if (vertices.empty() || indices.empty()) return nullptr;

    auto skeleton = std::make_shared<Skeleton>();
    std::unordered_map<std::string, int> boneMapping;

    //메시의 본 오프셋 행렬 매핑 테이블 생성
    for (unsigned int m = 0; m < scene->mNumMeshes; ++m)
    {
        const aiMesh* mesh = scene->mMeshes[m];
        for (unsigned int b = 0; b < mesh->mNumBones; ++b)
        {
            std::string boneName = mesh->mBones[b]->mName.C_Str();
            if (boneMapping.find(boneName) == boneMapping.end())
            {
                int newIdx = static_cast<int>(skeleton->Bones.size());
                boneMapping[boneName] = newIdx;

                skeleton->Bones.emplace_back();
                auto& bone = skeleton->Bones.back();
                bone.Name = boneName;
                bone.OffsetMatrix = AssimpMatrixToOurMatrix(mesh->mBones[b]->mOffsetMatrix);
                bone.ParentIndex = -1;
                bone.LocalBindTransform = Matrix4x4::Identity;
            }
        }
    }

    //노드 트리를 순회하며 부모 관계 및 바인드 TRS 세팅
    auto MapHierarchy = [&](auto& self, const aiNode* node, int parentIdx) -> void
        {
            std::string name = node->mName.C_Str();
            int currentIdx = parentIdx;

            if (boneMapping.find(name) != boneMapping.end())
            {
                currentIdx = boneMapping[name];
                auto& bone = skeleton->Bones[currentIdx];
                bone.ParentIndex = parentIdx;
                bone.LocalBindTransform = AssimpMatrixToOurMatrix(node->mTransformation);

                aiVector3D scale, pos;
                aiQuaternion rot;
                node->mTransformation.Decompose(scale, rot, pos);
                bone.BindPosition = Vector3(pos.x, pos.y, pos.z);
                bone.BindRotation = Quaternion(rot.x, rot.y, rot.z, rot.w);
                bone.BindScale = Vector3(scale.x, scale.y, scale.z);
            }

            for (unsigned int c = 0; c < node->mNumChildren; ++c)
            {
                self(self, node->mChildren[c], currentIdx);
            }
        };

    MapHierarchy(MapHierarchy, scene->mRootNode, -1);

    //정점에 본 인덱스와 가중치 할당
    for (unsigned int m = 0; m < scene->mNumMeshes; ++m)
    {
        const aiMesh* mesh = scene->mMeshes[m];
        UINT32 baseOffset = meshOffsets[m];

        for (unsigned int b = 0; b < mesh->mNumBones; ++b)
        {
            const aiBone* bone = mesh->mBones[b];
            int bIdx = boneMapping[bone->mName.C_Str()];

            for (unsigned int w = 0; w < bone->mNumWeights; ++w)
            {
                UINT32 targetVertexId = baseOffset + bone->mWeights[w].mVertexId;
                float weight = bone->mWeights[w].mWeight;

                for (int slot = 0; slot < 4; ++slot)
                {
                    if (vertices[targetVertexId].boneWeights[slot] == 0.0f)
                    {
                        vertices[targetVertexId].boneIndices[slot] = static_cast<UINT>(bIdx);
                        vertices[targetVertexId].boneWeights[slot] = weight;
                        break;
                    }
                }
            }
        }
    }

    //가중치 정규화
    for (auto& v : vertices)
    {
        float sum = v.boneWeights[0] + v.boneWeights[1] + v.boneWeights[2] + v.boneWeights[3];
        if (sum > 0.0001f)
        {
            v.boneWeights[0] /= sum;
            v.boneWeights[1] /= sum;
            v.boneWeights[2] /= sum;
            v.boneWeights[3] /= sum;
        }
        else
        {
            v.boneIndices[0] = 0;
            v.boneWeights[0] = 1.0f;
        }
    }

    //애니메이션 트랙 파싱
    if (outAnimations && scene->HasAnimations())
    {
        for (unsigned int a = 0; a < scene->mNumAnimations; ++a)
        {
            const aiAnimation* srcAnim = scene->mAnimations[a];
            auto animation = std::make_shared<Animation>();
            animation->Name = (srcAnim->mName.length > 0) ? srcAnim->mName.C_Str() : ("Animation_" + std::to_string(a));

            double ticks = srcAnim->mTicksPerSecond != 0.0 ? srcAnim->mTicksPerSecond : 25.0;
            animation->Duration = static_cast<float>(srcAnim->mDuration / ticks);

            animation->Channels.resize(skeleton->Bones.size());
            for (size_t i = 0; i < skeleton->Bones.size(); ++i)
            {
                animation->Channels[i].BoneIndex = static_cast<int>(i);
            }

            for (unsigned int c = 0; c < srcAnim->mNumChannels; ++c)
            {
                const aiNodeAnim* channel = srcAnim->mChannels[c];
                std::string target = channel->mNodeName.C_Str();

                if (boneMapping.find(target) == boneMapping.end()) continue;
                int bIdx = boneMapping[target];

                BoneKeyframes& keyframes = animation->Channels[bIdx];

                for (unsigned int k = 0; k < channel->mNumPositionKeys; ++k)
                {
                    keyframes.PositionTimes.push_back(static_cast<float>(channel->mPositionKeys[k].mTime / ticks));
                    aiVector3D p = channel->mPositionKeys[k].mValue;
                    keyframes.Positions.push_back(Vector3(p.x, p.y, p.z));
                }

                for (unsigned int k = 0; k < channel->mNumRotationKeys; ++k)
                {
                    keyframes.RotationTimes.push_back(static_cast<float>(channel->mRotationKeys[k].mTime / ticks));
                    aiQuaternion r = channel->mRotationKeys[k].mValue;
                    keyframes.Rotations.push_back(Quaternion(r.x, r.y, r.z, r.w));
                }

                for (unsigned int k = 0; k < channel->mNumScalingKeys; ++k)
                {
                    keyframes.ScaleTimes.push_back(static_cast<float>(channel->mScalingKeys[k].mTime / ticks));
                    aiVector3D s = channel->mScalingKeys[k].mValue;
                    keyframes.Scales.push_back(Vector3(s.x, s.y, s.z));
                }
            }

            outAnimations->push_back(animation);
        }
    }

    auto result = std::make_shared<SkinnedMesh>();
    result->Create(device, uploadContext, vertices, indices);
    result->SetSubMeshes(std::move(subMeshes));
    result->SetSkeleton(skeleton);
    result->SetBoundingBox(CalculateBounds(vertices));

    return result;
}