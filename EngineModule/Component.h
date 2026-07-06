#pragma once

class D3D12Renderer;
class GameObject;

class Component
{
public:
    virtual ~Component() = default;

    virtual void Update(float deltaTime) {}
    virtual void Render(D3D12Renderer& renderer) {}

    void SetOwner(GameObject* owner) { mOwner = owner; }
    GameObject* GetOwner() const { return mOwner; }

protected:
    GameObject* mOwner = nullptr;
};