// SceneSerializer.cpp
#include "pch.h"
#include "SceneSerializer.h"
#include "Scene.h"
#include "GameObject.h"
#include "Transform.h"
#include "Application.h"
#include <fstream>
#include <sstream>

bool SceneSerializer::Save(Scene& scene, const std::string& path)
{
    std::ofstream file(path);
    if (!file.is_open())
    {
        return false;
    }

    for (auto& obj : scene.GetGameObjects())
    {
        if (obj->GetSourceAssetPath().empty())
        {
            continue; // 프리미티브(큐브, 카메라 등)는 저장 대상 아님
        }

        Transform& t = obj->GetTransform();
        Vector3 pos = t.GetPosition();
        Vector3 rot = t.GetEulerAngles();
        Vector3 scale = t.GetScale();

        file << "GameObject\n";
        file << "Name " << obj->GetName() << "\n";
        file << "AssetPath " << obj->GetSourceAssetPath() << "\n";
        file << "Position " << pos.X << " " << pos.Y << " " << pos.Z << "\n";
        file << "Rotation " << rot.X << " " << rot.Y << " " << rot.Z << "\n";
        file << "Scale " << scale.X << " " << scale.Y << " " << scale.Z << "\n";
        file << "EndGameObject\n";
    }

    return true;
}

bool SceneSerializer::Load(Scene& scene, Application& app, const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        return false;
    }

    scene.ClearAssetObjects(); // 먼저 기존 에셋 오브젝트 전부 제거

    std::string line;
    std::string name, assetPath;
    Vector3 pos, rot, scale(1.0f, 1.0f, 1.0f);

    while (std::getline(file, line))
    {
        std::istringstream ss(line);
        std::string token;
        ss >> token;

        if (token == "GameObject")
        {
            name.clear();
            assetPath.clear();
            pos = Vector3::Zero;
            rot = Vector3::Zero;
            scale = Vector3::One;
        }
        else if (token == "Name")
        {
            std::getline(ss, name);
            if (!name.empty() && name[0] == ' ') name.erase(0, 1);
        }
        else if (token == "AssetPath")
        {
            ss >> assetPath;
        }
        else if (token == "Position")
        {
            ss >> pos.X >> pos.Y >> pos.Z;
        }
        else if (token == "Rotation")
        {
            ss >> rot.X >> rot.Y >> rot.Z;
        }
        else if (token == "Scale")
        {
            ss >> scale.X >> scale.Y >> scale.Z;
        }
        else if (token == "EndGameObject")
        {
            auto obj = app.SpawnAssetIntoScene(assetPath);
            if (obj)
            {
                obj->SetName(name);
                obj->GetTransform().SetPosition(pos);
                obj->GetTransform().SetRotation(rot);
                obj->GetTransform().SetScale(scale);
            }
        }
    }

    return true;
}