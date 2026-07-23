#pragma once
#include <memory>

class GameObject;

class EditorState
{
public:
    void SetSelected(std::shared_ptr<GameObject> obj) { mSelected = obj; }
    std::shared_ptr<GameObject> GetSelected() const { return mSelected.lock(); }

    //Viewport Focus 상태 관리를 위한 메서드
    bool IsViewportFocused() const { return mIsViewportFocused; }
    void SetViewportFocused(bool focused) { mIsViewportFocused = focused; }

private:
    std::weak_ptr<GameObject> mSelected; //Editor는 참조만 수행
    bool mIsViewportFocused = false;     //Viewport 포커스 여부
};