#pragma once

struct Vertex
{
    float position[3];
    float normal[3];
    float color[4];
    float uv[2];
    UINT boneIndices[4] = { 0, 0, 0, 0 };
    float boneWeights[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
};