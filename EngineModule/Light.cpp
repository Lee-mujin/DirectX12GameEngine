// LightPanel.cpp
#include "pch.h"
#include "Light.h"
#include "Scene.h"
#include "DirectionalLight.h"
#include "PointLight.h"
#include "SpotLight.h"
#include "imgui.h"

void Light::Draw(Scene& scene)
{
    ImGui::Begin("Lights");

    if (ImGui::CollapsingHeader("Directional Light", ImGuiTreeNodeFlags_DefaultOpen))
    {
        DirectionalLight& light = scene.GetMainLight();

        ImGui::PushID("DirLight");

        ImGui::ColorEdit3("Color", &light.Color.X);
        ImGui::DragFloat("Intensity", &light.Intensity, 0.05f, 0.0f, 10.0f);

        Vector3 direction = light.Direction;
        if (ImGui::DragFloat3("Direction", &direction.X, 0.01f, -1.0f, 1.0f))
        {
            light.Direction = direction.GetNormalize();
        }

        ImGui::PopID();
    }

    if (ImGui::CollapsingHeader("Point Light", ImGuiTreeNodeFlags_DefaultOpen))
    {
        PointLight& light = scene.GetPointLight();

        ImGui::PushID("PointLight");

        ImGui::Checkbox("Enabled", &light.Enabled);
        ImGui::ColorEdit3("Color", &light.Color.X);
        ImGui::DragFloat("Intensity", &light.Intensity, 0.05f, 0.0f, 20.0f);
        ImGui::DragFloat3("Position", &light.Position.X, 0.1f);
        ImGui::DragFloat("Range", &light.Range, 0.1f, 0.1f, 50.0f);

        ImGui::PopID();
    }

    if (ImGui::CollapsingHeader("Spot Light", ImGuiTreeNodeFlags_DefaultOpen))
    {
        SpotLight& light = scene.GetSpotLight();

        ImGui::PushID("SpotLight");

        ImGui::Checkbox("Enabled", &light.Enabled);
        ImGui::ColorEdit3("Color", &light.Color.X);
        ImGui::DragFloat("Intensity", &light.Intensity, 0.05f, 0.0f, 20.0f);
        ImGui::DragFloat3("Position", &light.Position.X, 0.1f);

        Vector3 direction = light.Direction;
        if (ImGui::DragFloat3("Direction", &direction.X, 0.01f, -1.0f, 1.0f))
        {
            light.Direction = direction.GetNormalize();
        }

        ImGui::DragFloat("Range", &light.Range, 0.1f, 0.1f, 50.0f);
        ImGui::DragFloat("Inner Cone Angle", &light.InnerConeAngle, 0.5f, 0.0f, light.OuterConeAngle);
        ImGui::DragFloat("Outer Cone Angle", &light.OuterConeAngle, 0.5f, light.InnerConeAngle, 90.0f);

        ImGui::PopID();
    }

    ImGui::End();
}