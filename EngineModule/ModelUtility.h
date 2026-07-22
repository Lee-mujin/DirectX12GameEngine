#pragma once
#include <memory>
#include "AssetHandle.h"

class GameObject;
class Model;
class ResourceManager;

class ModelUtility
{
public:
    //씬 로드, 런타임, Undo/Redo, 인스펙터 등 어디서나 호출 가능한 공용 모델 교체 함수
    static void ReplaceModel(GameObject& targetObject, std::shared_ptr<Model> model, AssetHandle sourceHandle = AssetHandle());
};