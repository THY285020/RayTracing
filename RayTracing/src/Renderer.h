#pragma once
#include "Walnut/Image.h"
#include "Camera.h"
#include "Ray.h"
#include "Scene.h"

#include <memory>
#include <glm/glm.hpp>
class Renderer
{
public:
	//设置面板的属性
	struct Settings
	{
		bool Accumulate = true;
		bool SlowRandom = false;
	};
	enum class Primitive
	{
		Sphere, 
		Quad
	};
	
	Renderer() = default;

	void Resize(uint32_t width, uint32_t height);
	void Render(Scene& scene, Camera& m_Camera);
	std::shared_ptr<Walnut::Image> GetFinalImage() const { return m_FinalImage; }

	void ResetFrameIndex() { m_FrameIndex = 1; }
	Settings& GetSettings() { return m_Settings; }
private:
	struct HitPayload
	{
		float HitDistance;
		glm::vec3 WorldPosition;
		glm::vec3 WorldNormal;
		std::shared_ptr<Object> Object;
		uint32_t ObjectIndex;
	};

	glm::vec4 PerPixel(uint32_t x, uint32_t y);//在Vulkan中的名称：RayGen
	HitPayload TraceRay(const Ray& ray);
	HitPayload ClosestHit(const Ray& ray, float hitDistance, Primitive hitPrimitive, std::shared_ptr<Object> object);
	HitPayload Miss(const Ray& ray);
private:
	std::shared_ptr<Walnut::Image> m_FinalImage;
	Settings m_Settings;

	const Scene* m_ActiveScene = nullptr;
	const Camera* m_ActiveCamera = nullptr;

	std::shared_ptr<uint32_t[]> m_ImageData;
	std::shared_ptr<glm::vec4[]> m_AccumulationData;

	//uint32_t* m_ImageData = nullptr;
	//glm::vec4* m_AccumulationData = nullptr;

	uint32_t m_FrameIndex = 1;

	std::vector<uint32_t> m_HorizontalIterator, m_VerticalIterator;
};

