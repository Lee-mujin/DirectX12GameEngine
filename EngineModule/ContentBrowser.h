#pragma once
#include <string>
#include <functional>
#include <filesystem>

struct FileEntry
{
    std::filesystem::path Path;
    std::string Name;
    std::string Extension;
    bool IsDirectory = false;
};

class ContentBrowser
{
public:
    // path, extension(".glb" 등)을 넘겨줌 — 실제 로드는 이 콜백을 등록한 쪽(Application)이 판단
    using ActivateCallback = std::function<void(const std::filesystem::path&)>;

    void Initialize(const std::string& rootPath, ActivateCallback onActivate);
    void Draw();

private:
    void DrawDirectory(const std::filesystem::path& dirPath);

private:
    std::filesystem::path mRootPath;
    ActivateCallback mOnActivate;
};