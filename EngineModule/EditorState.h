#pragma once
#include <memory>

class GameObject;

class EditorState
{
public:
    void SetSelected(std::shared_ptr<GameObject> obj) { mSelected = obj; }
    std::shared_ptr<GameObject> GetSelected() const { return mSelected.lock(); }

private:
    std::weak_ptr<GameObject> mSelected; //Editor는 그냥 참조만
};