#pragma once

class Scene;

//에디터 UI에서 씬의 광원 속성(Directional, Point, Spot)을 편집하는 인스펙션 패널
class LightPanel
{
public:
    void Draw(Scene& scene);
};