#include "pch.h"
#include "ModelLoader.h"
#include "Mesh.h"
#include "Vertex.h"
#include "../GameEngine/ThirdParty/cgltf.h"
#include "SkinnedMesh.h"
#include "Skeleton.h"
#include "Animation.h"
#include "UploadContext.h"
#include <vector>
#include <cmath>

namespace
{
    Matrix4x4 CgltfMatrixToOurMatrix(const cgltf_float m[16])
    {
        return Matrix4x4(
            m[0], m[1], m[2], m[3],
            m[4], m[5], m[6], m[7],
            m[8], m[9], m[10], m[11],
            m[12], m[13], m[14], m[15]);
    }

    //Tangent가 없는 파일일 경우 Lengyel's Method(Gram-Schmidt 직교화)로 계산
    void ComputeTangentsIfMissing(std::vector<Vertex>& vertices, const std::vector<UINT16>& indices, bool hasTangents)
    {
        if (hasTangents)
        {
            return; // 파일에 이미 있으면 계산 불필요
        }

        std::vector<Vector3> tan1(vertices.size(), Vector3::Zero);
        std::vector<Vector3> tan2(vertices.size(), Vector3::Zero);

        for (size_t i = 0; i + 2 < indices.size(); i += 3)
        {
            UINT16 i0 = indices[i], i1 = indices[i + 1], i2 = indices[i + 2];

            Vector3 p0(vertices[i0].position[0], vertices[i0].position[1], vertices[i0].position[2]);
            Vector3 p1(vertices[i1].position[0], vertices[i1].position[1], vertices[i1].position[2]);
            Vector3 p2(vertices[i2].position[0], vertices[i2].position[1], vertices[i2].position[2]);

            Vector2 uv0(vertices[i0].uv[0], vertices[i0].uv[1]);
            Vector2 uv1(vertices[i1].uv[0], vertices[i1].uv[1]);
            Vector2 uv2(vertices[i2].uv[0], vertices[i2].uv[1]);

            Vector3 edge1 = p1 - p0;
            Vector3 edge2 = p2 - p0;
            float du1 = uv1.X - uv0.X, dv1 = uv1.Y - uv0.Y;
            float du2 = uv2.X - uv0.X, dv2 = uv2.Y - uv0.Y;

            float denom = du1 * dv2 - du2 * dv1;
            if (std::fabsf(denom) < 0.0001f)
            {
                continue; // UV 퇴화 삼각형, 스킵
            }
            float r = 1.0f / denom;

            Vector3 sdir = (edge1 * dv2 - edge2 * dv1) * r;
            Vector3 tdir = (edge2 * du1 - edge1 * du2) * r;

            tan1[i0] += sdir; tan1[i1] += sdir; tan1[i2] += sdir;
            tan2[i0] += tdir; tan2[i1] += tdir; tan2[i2] += tdir;
        }

        for (size_t i = 0; i < vertices.size(); ++i)
        {
            Vector3 n(vertices[i].normal[0], vertices[i].normal[1], vertices[i].normal[2]);
            Vector3 t = tan1[i]; // Gram-Schmidt 직교화: normal 성분 제거

            // t가 Zero 벡터인 경우(UV가 비어있거나 문제 있는 경우) 방지
            if (t.GetSizeSq() < 0.0001f)
            {
                vertices[i].tangent[0] = 1.0f;
                vertices[i].tangent[1] = 0.0f;
                vertices[i].tangent[2] = 0.0f;
                vertices[i].tangent[3] = 1.0f;
                continue;
            }

            Vector3 tangent = (t - n * Vector3::Dot(n, t)).GetNormalize();

            float handedness = (Vector3::Dot(Vector3::Cross(n, tangent), tan2[i]) < 0.0f) ? -1.0f : 1.0f;

            vertices[i].tangent[0] = tangent.X;
            vertices[i].tangent[1] = tangent.Y;
            vertices[i].tangent[2] = tangent.Z;
            vertices[i].tangent[3] = handedness;
        }
    }
}

std::shared_ptr<Mesh> ModelLoader::LoadStaticMesh(ID3D12Device* device, UploadContext* uploadContext, const std::string& path)
{
    cgltf_options options = {};
    cgltf_data* data = nullptr;

    if (cgltf_parse_file(&options, path.c_str(), &data) != cgltf_result_success)
    {
        return nullptr;
    }

    if (cgltf_load_buffers(&options, data, path.c_str()) != cgltf_result_success)
    {
        cgltf_free(data);
        return nullptr;
    }

    if (data->meshes_count == 0 || data->meshes[0].primitives_count == 0)
    {
        cgltf_free(data);
        return nullptr;
    }

    const cgltf_primitive& primitive = data->meshes[0].primitives[0];

    std::vector<Vertex> vertices;
    std::vector<UINT16> indices;

    cgltf_accessor* positionAccessor = nullptr;
    cgltf_accessor* normalAccessor = nullptr;
    cgltf_accessor* tangentAccessor = nullptr;
    cgltf_accessor* uvAccessor = nullptr;

    for (size_t i = 0; i < primitive.attributes_count; ++i)
    {
        const cgltf_attribute& attr = primitive.attributes[i];

        if (attr.type == cgltf_attribute_type_position)
        {
            positionAccessor = attr.data;
        }
        else if (attr.type == cgltf_attribute_type_normal)
        {
            normalAccessor = attr.data;
        }
        else if (attr.type == cgltf_attribute_type_tangent)
        {
            tangentAccessor = attr.data;
        }
        else if (attr.type == cgltf_attribute_type_texcoord && uvAccessor == nullptr)
        {
            uvAccessor = attr.data;
        }
    }

    if (!positionAccessor)
    {
        cgltf_free(data);
        return nullptr;
    }

    size_t vertexCount = positionAccessor->count;
    vertices.resize(vertexCount);

    for (size_t i = 0; i < vertexCount; ++i)
    {
        float pos[3] = {};
        cgltf_accessor_read_float(positionAccessor, i, pos, 3);
        vertices[i].position[0] = pos[0];
        vertices[i].position[1] = pos[1];
        vertices[i].position[2] = pos[2];

        if (normalAccessor)
        {
            float normal[3] = {};
            cgltf_accessor_read_float(normalAccessor, i, normal, 3);
            vertices[i].normal[0] = normal[0];
            vertices[i].normal[1] = normal[1];
            vertices[i].normal[2] = normal[2];
        }

        if (tangentAccessor)
        {
            float tan[4] = {};
            cgltf_accessor_read_float(tangentAccessor, i, tan, 4);
            vertices[i].tangent[0] = tan[0];
            vertices[i].tangent[1] = tan[1];
            vertices[i].tangent[2] = tan[2];
            vertices[i].tangent[3] = tan[3];
        }

        if (uvAccessor)
        {
            float uv[2] = {};
            cgltf_accessor_read_float(uvAccessor, i, uv, 2);
            vertices[i].uv[0] = uv[0];
            vertices[i].uv[1] = uv[1];
        }

        vertices[i].color[0] = 1.0f;
        vertices[i].color[1] = 1.0f;
        vertices[i].color[2] = 1.0f;
        vertices[i].color[3] = 1.0f;
    }

    if (primitive.indices)
    {
        size_t indexCount = primitive.indices->count;
        indices.resize(indexCount);

        for (size_t i = 0; i < indexCount; ++i)
        {
            indices[i] = static_cast<UINT16>(cgltf_accessor_read_index(primitive.indices, i));
        }
    }

    cgltf_free(data);

    //메쉬 생성 직전, Tangent 데이터가 없으면 자동 계산
    ComputeTangentsIfMissing(vertices, indices, tangentAccessor != nullptr);

    auto result = std::make_shared<Mesh>();
    result->Create(device, uploadContext, vertices, indices);
    return result;
}

std::shared_ptr<SkinnedMesh> ModelLoader::LoadSkinnedMesh(
    ID3D12Device* device, UploadContext* uploadContext, const std::string& path,
    std::vector<std::shared_ptr<Animation>>* outAnimations)
{
    if (outAnimations)
    {
        outAnimations->clear();
    }

    cgltf_options options = {};
    cgltf_data* data = nullptr;

    if (cgltf_parse_file(&options, path.c_str(), &data) != cgltf_result_success)
    {
        return nullptr;
    }

    if (cgltf_load_buffers(&options, data, path.c_str()) != cgltf_result_success)
    {
        cgltf_free(data);
        return nullptr;
    }

    if (data->meshes_count == 0 || data->meshes[0].primitives_count == 0 || data->skins_count == 0)
    {
        cgltf_free(data);
        return nullptr;
    }

    const cgltf_primitive& primitive = data->meshes[0].primitives[0];
    const cgltf_skin& skin = data->skins[0];

    // Skeleton 구성
    auto skeleton = std::make_shared<Skeleton>();
    skeleton->Bones.resize(skin.joints_count);

    for (size_t i = 0; i < skin.joints_count; ++i)
    {
        cgltf_node* joint = skin.joints[i];
        skeleton->Bones[i].Name = joint->name ? joint->name : "";

        float offsetMat[16] = {};
        cgltf_accessor_read_float(skin.inverse_bind_matrices, i, offsetMat, 16);
        skeleton->Bones[i].OffsetMatrix = CgltfMatrixToOurMatrix(offsetMat);

        Vector3 pos(joint->translation[0], joint->translation[1], joint->translation[2]);
        Quaternion rot(joint->rotation[0], joint->rotation[1], joint->rotation[2], joint->rotation[3]);
        rot.Normalize();
        Vector3 scale(joint->scale[0], joint->scale[1], joint->scale[2]);

        skeleton->Bones[i].LocalBindTransform =
            Matrix4x4::Scale(scale) * Matrix4x4::Rotate(rot) * Matrix4x4::Translate(pos);

        skeleton->Bones[i].BindPosition = pos;
        skeleton->Bones[i].BindRotation = rot;
        skeleton->Bones[i].BindScale = scale;

        skeleton->Bones[i].ParentIndex = -1;

        cgltf_node* searchParent = joint->parent;
        while (searchParent != nullptr)
        {
            bool found = false;
            for (size_t p = 0; p < skin.joints_count; ++p)
            {
                if (skin.joints[p] == searchParent)
                {
                    skeleton->Bones[i].ParentIndex = static_cast<int>(p);
                    found = true;
                    break;
                }
            }
            if (found) break;
            searchParent = searchParent->parent;
        }
    }

    // 정점 데이터
    cgltf_accessor* positionAccessor = nullptr;
    cgltf_accessor* normalAccessor = nullptr;
    cgltf_accessor* tangentAccessor = nullptr;
    cgltf_accessor* uvAccessor = nullptr;
    cgltf_accessor* jointsAccessor = nullptr;
    cgltf_accessor* weightsAccessor = nullptr;

    for (size_t i = 0; i < primitive.attributes_count; ++i)
    {
        const cgltf_attribute& attr = primitive.attributes[i];
        switch (attr.type)
        {
        case cgltf_attribute_type_position: positionAccessor = attr.data; break;
        case cgltf_attribute_type_normal: normalAccessor = attr.data; break;
        case cgltf_attribute_type_tangent: tangentAccessor = attr.data; break;
        case cgltf_attribute_type_texcoord: if (!uvAccessor) uvAccessor = attr.data; break;
        case cgltf_attribute_type_joints: if (!jointsAccessor) jointsAccessor = attr.data; break;
        case cgltf_attribute_type_weights: if (!weightsAccessor) weightsAccessor = attr.data; break;
        default: break;
        }
    }

    if (!positionAccessor || !jointsAccessor || !weightsAccessor)
    {
        cgltf_free(data);
        return nullptr;
    }

    size_t vertexCount = positionAccessor->count;
    std::vector<Vertex> vertices(vertexCount);

    for (size_t i = 0; i < vertexCount; ++i)
    {
        float pos[3] = {};
        cgltf_accessor_read_float(positionAccessor, i, pos, 3);
        vertices[i].position[0] = pos[0];
        vertices[i].position[1] = pos[1];
        vertices[i].position[2] = pos[2];

        if (normalAccessor)
        {
            float normal[3] = {};
            cgltf_accessor_read_float(normalAccessor, i, normal, 3);
            vertices[i].normal[0] = normal[0];
            vertices[i].normal[1] = normal[1];
            vertices[i].normal[2] = normal[2];
        }

        if (tangentAccessor)
        {
            float tan[4] = {};
            cgltf_accessor_read_float(tangentAccessor, i, tan, 4);
            vertices[i].tangent[0] = tan[0];
            vertices[i].tangent[1] = tan[1];
            vertices[i].tangent[2] = tan[2];
            vertices[i].tangent[3] = tan[3]; // handedness
        }

        if (uvAccessor)
        {
            float uv[2] = {};
            cgltf_accessor_read_float(uvAccessor, i, uv, 2);
            vertices[i].uv[0] = uv[0];
            vertices[i].uv[1] = uv[1];
        }

        vertices[i].color[0] = vertices[i].color[1] = vertices[i].color[2] = vertices[i].color[3] = 1.0f;

        cgltf_uint joints[4] = {};
        cgltf_accessor_read_uint(jointsAccessor, i, joints, 4);
        vertices[i].boneIndices[0] = joints[0];
        vertices[i].boneIndices[1] = joints[1];
        vertices[i].boneIndices[2] = joints[2];
        vertices[i].boneIndices[3] = joints[3];

        float weights[4] = {};
        cgltf_accessor_read_float(weightsAccessor, i, weights, 4);

        float weightSum = weights[0] + weights[1] + weights[2] + weights[3];
        if (weightSum > 0.0001f)
        {
            weights[0] /= weightSum;
            weights[1] /= weightSum;
            weights[2] /= weightSum;
            weights[3] /= weightSum;
        }

        vertices[i].boneWeights[0] = weights[0];
        vertices[i].boneWeights[1] = weights[1];
        vertices[i].boneWeights[2] = weights[2];
        vertices[i].boneWeights[3] = weights[3];
    }

    std::vector<UINT16> indices;
    if (primitive.indices)
    {
        size_t indexCount = primitive.indices->count;
        indices.resize(indexCount);
        for (size_t i = 0; i < indexCount; ++i)
        {
            indices[i] = static_cast<UINT16>(cgltf_accessor_read_index(primitive.indices, i));
        }
    }

    // 애니메이션
    if (outAnimations && data->animations_count > 0)
    {
        for (size_t a = 0; a < data->animations_count; ++a)
        {
            const cgltf_animation& srcAnim = data->animations[a];

            auto animation = std::make_shared<Animation>();
            animation->Name = srcAnim.name ? srcAnim.name : ("Animation_" + std::to_string(a));
            animation->Channels.resize(skeleton->Bones.size());
            for (size_t i = 0; i < skeleton->Bones.size(); ++i)
            {
                animation->Channels[i].BoneIndex = static_cast<int>(i);
            }

            float maxTime = 0.0f;
            int matchedChannelCount = 0;

            for (size_t c = 0; c < srcAnim.channels_count; ++c)
            {
                const cgltf_animation_channel& channel = srcAnim.channels[c];
                const cgltf_animation_sampler& sampler = *channel.sampler;
                cgltf_node* targetNode = channel.target_node;

                int boneIndex = -1;
                for (size_t b = 0; b < skin.joints_count; ++b)
                {
                    if (skin.joints[b] == targetNode)
                    {
                        boneIndex = static_cast<int>(b);
                        break;
                    }
                }

                if (boneIndex < 0)
                {
                    continue;
                }
                ++matchedChannelCount;

                size_t keyCount = sampler.input->count;
                std::vector<float> times(keyCount);
                for (size_t k = 0; k < keyCount; ++k)
                {
                    cgltf_accessor_read_float(sampler.input, k, &times[k], 1);
                    maxTime = (times[k] > maxTime) ? times[k] : maxTime;
                }

                BoneKeyframes& boneChannel = animation->Channels[boneIndex];

                if (channel.target_path == cgltf_animation_path_type_translation)
                {
                    boneChannel.PositionTimes = times;
                    boneChannel.Positions.resize(keyCount);
                    for (size_t k = 0; k < keyCount; ++k)
                    {
                        float v[3] = {};
                        cgltf_accessor_read_float(sampler.output, k, v, 3);
                        boneChannel.Positions[k] = Vector3(v[0], v[1], v[2]);
                    }
                }
                else if (channel.target_path == cgltf_animation_path_type_rotation)
                {
                    boneChannel.RotationTimes = times;
                    boneChannel.Rotations.resize(keyCount);
                    for (size_t k = 0; k < keyCount; ++k)
                    {
                        float v[4] = {};
                        cgltf_accessor_read_float(sampler.output, k, v, 4);
                        boneChannel.Rotations[k] = Quaternion(v[0], v[1], v[2], v[3]);
                    }
                }
                else if (channel.target_path == cgltf_animation_path_type_scale)
                {
                    boneChannel.ScaleTimes = times;
                    boneChannel.Scales.resize(keyCount);
                    for (size_t k = 0; k < keyCount; ++k)
                    {
                        float v[3] = {};
                        cgltf_accessor_read_float(sampler.output, k, v, 3);
                        boneChannel.Scales[k] = Vector3(v[0], v[1], v[2]);
                    }
                }
            }

            animation->Duration = maxTime;

            if (matchedChannelCount > 0)
            {
                outAnimations->push_back(animation);
            }
        }
    }

    cgltf_free(data);

    //스키니드 메쉬 생성 직전, Tangent 데이터가 없으면 자동 계산
    ComputeTangentsIfMissing(vertices, indices, tangentAccessor != nullptr);

    auto result = std::make_shared<SkinnedMesh>();
    result->Create(device, uploadContext, vertices, indices);
    result->SetSkeleton(skeleton);

    return result;
}