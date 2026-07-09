// ModelLoader.cpp
#include "pch.h"
#include "ModelLoader.h"
#include "Mesh.h"
#include "Vertex.h"
#include "../GameEngine/ThirdParty/cgltf.h"
#include "SkinnedMesh.h"
#include "Skeleton.h"
#include <vector>

namespace
{
    Matrix4x4 CgltfMatrixToOurMatrix(const cgltf_float m[16])
    {
        // cgltf는 열우선(column-major) 4x4를 float[16]으로 줌
        return Matrix4x4(
            m[0], m[4], m[8], m[12],
            m[1], m[5], m[9], m[13],
            m[2], m[6], m[10], m[14],
            m[3], m[7], m[11], m[15]);
    }
}

std::shared_ptr<Mesh> ModelLoader::LoadStaticMesh(ID3D12Device* device, const std::string& path)
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

    const cgltf_primitive& primitive = data->meshes[0].primitives[0]; // 1단계: 첫 메시의 첫 프리미티브만

    std::vector<Vertex> vertices;
    std::vector<UINT16> indices;

    cgltf_accessor* positionAccessor = nullptr;
    cgltf_accessor* normalAccessor = nullptr;
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

    auto result = std::make_shared<Mesh>();
    result->Create(device, vertices, indices);
    return result;
}

std::shared_ptr<SkinnedMesh> ModelLoader::LoadSkinnedMesh(ID3D12Device* device, const std::string& path)
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

    if (data->meshes_count == 0 || data->meshes[0].primitives_count == 0 || data->skins_count == 0)
    {
        cgltf_free(data);
        return nullptr;
    }

    const cgltf_primitive& primitive = data->meshes[0].primitives[0];
    const cgltf_skin& skin = data->skins[0];

    // 1) Skeleton 구성
    auto skeleton = std::make_shared<Skeleton>();
    skeleton->Bones.resize(skin.joints_count);

    for (size_t i = 0; i < skin.joints_count; ++i)
    {
        cgltf_node* joint = skin.joints[i];
        skeleton->Bones[i].Name = joint->name ? joint->name : "";

        float offsetMat[16] = {};
        cgltf_accessor_read_float(skin.inverse_bind_matrices, i, offsetMat, 16);
        skeleton->Bones[i].OffsetMatrix = CgltfMatrixToOurMatrix(offsetMat);

        skeleton->Bones[i].ParentIndex = -1;
        for (size_t p = 0; p < skin.joints_count; ++p)
        {
            if (skin.joints[p] == joint->parent)
            {
                skeleton->Bones[i].ParentIndex = static_cast<int>(p);
                break;
            }
        }
    }

    // 2) 정점 데이터 (POSITION/NORMAL/UV + JOINTS/WEIGHTS)
    cgltf_accessor* positionAccessor = nullptr;
    cgltf_accessor* normalAccessor = nullptr;
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

    cgltf_free(data);

    auto result = std::make_shared<SkinnedMesh>();
    result->Create(device, vertices, indices);
    result->SetSkeleton(skeleton);
    return result;
}