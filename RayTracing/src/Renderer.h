#pragma once
#include "Walnut/Image.h"

#include <memory>
#include <glm/glm.hpp>
class Renderer
{
public:
	Renderer() = default;

	void Resize(uint32_t width, uint32_t height);
	void Render();
	glm::vec4 PerPixel(glm::vec2 coord);
	std::shared_ptr<Walnut::Image> GetFinalImage() const { return m_FinalImage; }

private:
	std::shared_ptr<Walnut::Image> m_FinalImage;
	uint32_t* m_ImageData = nullptr;
};
