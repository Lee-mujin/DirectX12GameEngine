#pragma once
#include <vector>
#include <memory>
#include "Transform.h"
#include "Component.h"
#include "AssetHandle.h"

class D3D12Renderer;

class GameObject
{
public:
    virtual ~GameObject() = default;

    virtual void Update(float deltaTime);
    virtual void Render(D3D12Renderer& renderer);

    template<typename T, typename... Args>
    T* AddComponent(Args&&... args)
    {
        auto component = std::make_unique<T>(std::forward<Args>(args)...);
        component->SetOwner(this);
        T* ptr = component.get();
        mComponents.push_back(std::move(component));
        return ptr;
    }

    template<typename T>
    T* GetComponent()
    {
        for (auto& c : mComponents)
        {
            if (T* casted = dynamic_cast<T*>(c.get()))
            {
                return casted;
            }
        }
        return nullptr;
    }

    Transform& GetTransform() { return mTransform; }
    const Transform& GetTransform() const { return mTransform; }
    const std::string& GetName() const { return mName; }
    AssetHandle GetSourceAssetHandle() const { return mSourceAssetHandle; }
    void SetSourceAssetHandle(AssetHandle handle) { mSourceAssetHandle = handle; }
    void SetName(const std::string& name) { mName = name; }

protected:
    Transform mTransform;
    std::vector<std::unique_ptr<Component>> mComponents;
private:
    std::string mName = "GameObject";
    AssetHandle mSourceAssetHandle; // 프리미티브(큐브 등)는 Invalid
};