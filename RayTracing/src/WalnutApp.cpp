#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"

#include "Walnut/Image.h"
#include "Walnut/Timer.h"

#include "Renderer.h"

#include <imgui_internal.h>
#include <glm/gtc/type_ptr.hpp>

class ExampleLayer : public Walnut::Layer
{
public:
	ExampleLayer() :m_Camera(45.0f, 0.1f, 100.f)
	{
		Material& pinkSphere = m_Scene.Materials.emplace_back();
		pinkSphere.Albedo = {1.0f, 0.0f, 1.0f};
		pinkSphere.Roughness = 0.0f;

		Material& blueSphere = m_Scene.Materials.emplace_back();
		blueSphere.Albedo = { 0.2f, 0.3f, 1.0f };
		blueSphere.Roughness = 0.1f;

		Material& orangeSphere = m_Scene.Materials.emplace_back();
		orangeSphere.Albedo = {0.8f, 0.5f, 0.2f};
		orangeSphere.Roughness = 1.0f;
		orangeSphere.EmissionColor = orangeSphere.Albedo;
		orangeSphere.EmissionPower = 2.0f;

		{
			Sphere sphere;
			sphere.Position = { 0.f, 0.f, 0.f };
			sphere.Radius = 1.0f;
			sphere.MaterialIndex = 0;
			m_Scene.Spheres.push_back(sphere);
		}

		{
			Sphere sphere;
			sphere.Position = { 0.0f, -101.0f, -5.0f };
			sphere.Radius = 100.0f;
			sphere.MaterialIndex = 1;
			m_Scene.Spheres.push_back(sphere);
		}

		{
			Sphere sphere;
			sphere.Position = { 2.0f, 0.0f, 0.0f };
			sphere.Radius = 1.0f;
			sphere.MaterialIndex = 2;
			m_Scene.Spheres.push_back(sphere);
		}

		{
			Quad quad;
			quad.Position = { 1.0f, 1.0f, 0.0f };
			quad.Rotation = { 0.0f, 45.0f, 0.0f };
			quad.Width = 1.0f;
			quad.Height = 1.0f;
			quad.MaterialIndex = 0;
			quad.Init();
			m_Scene.Quads.push_back(quad);
		}
	}
	virtual void OnUpdate(float ts) override
	{
		if(m_Camera.OnUpdate(ts))
			m_Renderer.ResetFrameIndex();
	}
	virtual void OnUIRender() override
	{
		
		ImGui::Begin("Settings");
		ImGui::Text("RenderTime: %.3fms", m_lastRenderTime);
		
		if (ImGui::Button("Render"))
		{
			Render();
		}

		ImGui::Checkbox("Accumulate", &m_Renderer.GetSettings().Accumulate);
		ImGui::Checkbox("SlowRandom", &m_Renderer.GetSettings().SlowRandom);

		if (ImGui::Button("Reset"))
			m_Renderer.ResetFrameIndex();
		ImGui::End();

		ImGui::Begin("Scene");
		for (size_t i = 0; i<m_Scene.Spheres.size(); ++i)
		{
			ImGui::PushID(i);

			Sphere& sphere = m_Scene.Spheres[i];
			ImGui::Text("Sphere %d", i);
			ImGui::DragFloat3("Position", glm::value_ptr(sphere.Position), 0.1f);
			ImGui::DragFloat("Radius", &sphere.Radius, 0.1f);
			ImGui::DragInt("Material", &sphere.MaterialIndex, 1.0f, 0, (int)m_Scene.Materials.size() - 1);
			
			ImGui::Separator();
			ImGui::PopID();
		}

		for (size_t i = 0; i < m_Scene.Quads.size(); ++i)
		{
			size_t index = i + m_Scene.Spheres.size();
			ImGui::PushID(index);

			Quad& quad = m_Scene.Quads[i];
			ImGui::Text("Quad %d", i);
			ImGui::DragFloat3("Position", glm::value_ptr(quad.Position), 0.1f);
			ImGui::DragFloat3("Rotation", glm::value_ptr(quad.Rotation), 0.1f);
			ImGui::DragFloat("Width", &quad.Width, 0.1f, 0.0f);
			ImGui::DragFloat("Height", &quad.Height, 0.1f, 0.0f);
			ImGui::Separator();
			ImGui::PopID();
			quad.Init();
		}

		ImGui::End();

		ImGui::Begin("Materials");
		for (size_t i = 0; i < m_Scene.Materials.size(); ++i)
		{
			ImGui::PushID(i);

			Material& material = m_Scene.Materials[i];
			ImGui::Text("Material%d", i);
			ImGui::ColorEdit3("Abedo", glm::value_ptr(material.Albedo));
			ImGui::DragFloat("Roughness", &material.Roughness, 0.05f, 0.0f, 1.0f);
			ImGui::DragFloat("Metallic", &material.Metallic, 0.05f, 0.0f, 1.0f);
			ImGui::ColorEdit3("EmissionColor", glm::value_ptr(material.EmissionColor));
			ImGui::DragFloat("EmissionPower", &material.EmissionPower, 0.05f, 0.0f, FLT_MAX);


			ImGui::Separator();
			ImGui::PopID();
		}
		ImGui::End();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("Viewport");

		m_ViewportWidth = ImGui::GetContentRegionAvail().x;
		m_ViewportHeight = ImGui::GetContentRegionAvail().y;

		auto image = m_Renderer.GetFinalImage();
		if (image)
		{
			ImGui::Image(image->GetDescriptorSet(), { (float)image->GetWidth(), (float)image->GetHeight()},
				ImVec2(0, 1), ImVec2(1,0));//uv0, uv1·´×ª×ø±ê
		}
		
		ImGui::End();
		ImGui::PopStyleVar();

		Render();
	}
private:
	void Render()
	{
		Walnut::Timer timer;

		m_Renderer.Resize(m_ViewportWidth, m_ViewportHeight);
		m_Camera.OnResize(m_ViewportWidth, m_ViewportHeight);
		m_Renderer.Render(m_Scene, m_Camera);

		m_lastRenderTime = timer.ElapsedMillis();
	}
private:
	Renderer m_Renderer;
	Camera m_Camera;
	Scene m_Scene;
	uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;
	float m_lastRenderTime;
};

Walnut::Application* Walnut::CreateApplication(int argc, char** argv)
{
	Walnut::ApplicationSpecification spec;
	spec.Name = "RayTracing";

	Walnut::Application* app = new Walnut::Application(spec);
	app->PushLayer<ExampleLayer>();
	app->SetMenubarCallback([app]()
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Exit"))
			{
				app->Close();
			}
			ImGui::EndMenu();
		}
	});
	return app;
}