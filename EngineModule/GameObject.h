#pragma once
#include <vector>
#include <memory>
#include <string>
#include <algorithm>
#include <typeindex>
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

    //컴포넌트 중복 생성 방지: 이미 존재하면 기존 포인터 반환
    template<typename T, typename... Args>
    T* AddComponent(Args&&... args)
    {
        if (T* existing = GetComponent<T>())
        {
            return existing;
        }

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

    //typeid(T) 기반의 빠른 타입 비교 및 제거
    template<typename T>
    bool RemoveComponent()
    {
        const std::type_index targetType = typeid(T);

        auto it = std::remove_if(mComponents.begin(), mComponents.end(),
            [targetType](const std::unique_ptr<Component>& c) {
                return std::type_index(typeid(*c)) == targetType;
            });

        if (it != mComponents.end())
        {
            mComponents.erase(it, mComponents.end());
            return true;
        }
        return false;
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
    AssetHandle mSourceAssetHandle;
};