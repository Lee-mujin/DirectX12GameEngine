#include "pch.h"

void GameObject::Update(float deltaTime)
{
    for (auto& c : mComponents)
    {
        c->Update(deltaTime);
    }
}
//렌더가 직접적으로 GameObject의 Transform을 사용하지 않고 각 Component가 자신의 Transform을 사용하여 렌더링
void GameObject::Render(D3D12Renderer& renderer)
{
    for (auto& c : mComponents)
    {
        c->Render(renderer);
    }
}