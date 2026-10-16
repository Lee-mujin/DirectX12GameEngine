#pragma once

class GameObject;

class Component
{
public:
    virtual ~Component() = default;

    virtual void Update(float deltaTime) {}
    void SetOwner(GameObject* owner) { mOwner = owner; }
    GameObject* GetOwner() const { return mOwner; }

protected:
    GameObject* mOwner = nullptr;
};