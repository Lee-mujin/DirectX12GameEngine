#pragma once
#include <vector>
#include "RenderItem.h"

//Scene과 Renderer 사이에서 드로우 패킷을 수집/분류하는 렌더 큐
class RenderQueue
{
public:
    void Clear()
    {
        mOpaqueItems.clear();
    }

    void Add(const RenderItem& item)
    {
        //알파 블렌딩/투명 오브젝트 확장 시 mTransparentItems 분류 가능
        mOpaqueItems.push_back(item);
    }

    const std::vector<RenderItem>& GetOpaqueItems() const { return mOpaqueItems; }
    bool IsEmpty() const { return mOpaqueItems.empty(); }

private:
    std::vector<RenderItem> mOpaqueItems;
};