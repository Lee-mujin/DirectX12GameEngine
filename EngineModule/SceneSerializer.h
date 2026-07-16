#pragma once
#include <string>

class Scene;
class Application;

class SceneSerializer
{
public:
    static bool Save(Scene& scene, const std::string& path);
    static bool Load(Scene& scene, Application& app, const std::string& path);
};