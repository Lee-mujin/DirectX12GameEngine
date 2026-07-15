#pragma once
#include <functional>
#include <filesystem>

class Scene;
class EditorState;

class Hierarchy
{
public:
    using AssetDropCallback = std::function<void(const std::filesystem::path&)>;

    void SetOnAssetDropped(AssetDropCallback callback) { mOnAssetDropped = callback; }
    void Draw(Scene& scene, EditorState& editorState);

private:
    AssetDropCallback mOnAssetDropped;
};