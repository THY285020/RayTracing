#include "Renderer.h"
#include "Walnut/Random.h"
#include <algorithm>

namespace utils
{
	static uint32_t Convert2RGBA(const glm::vec4& color)
	{
		uint8_t r = (uint8_t)(color.r * 255.0f);//映射到0-255
		uint8_t g = (uint8_t)(color.g * 255.0f);
		uint8_t b = (uint8_t)(color.b * 255.0f);
		uint8_t a = (uint8_t)(color.a * 255.0f);

		return (a << 24) | (b << 16) | (g << 8) | r;//ABGR 16进制=2^4占4位 高位存在低地址（大端存储）
	}
}
void Renderer::Resize(uint32_t width, uint32_t height)
{
	if (m_FinalImage)
	{
		//不需要调整大小
		if (m_FinalImage->GetWidth() == width && m_FinalImage->GetHeight() == height)
			return;

		m_FinalImage->Resize(width, height);
	}
	else
	{
		m_FinalImage = std::make_shared<Walnut::Image>(width, height, Walnut::ImageFormat::RGBA);
	}

	delete[] m_ImageData;
	m_ImageData = new uint32_t[width * height];
}

void Renderer::Render()
{
	float aspectRatio = m_FinalImage->GetWidth() / m_FinalImage->GetHeight();
	//更新缓冲
	for (uint32_t y = 0; y < m_FinalImage->GetHeight(); ++y)
	{
		for (uint32_t x = 0; x < m_FinalImage->GetWidth(); ++x)
		{
			//映射到0 1
			glm::vec2 coord = { (float)x / (float)m_FinalImage->GetWidth(), (float)y / (float)m_FinalImage->GetHeight() };
			coord = 2.0f * coord - 1.0f;//映射到-1 1

			//coord.x *= aspectRatio;

			m_ImageData[x + y* m_FinalImage->GetWidth()] = utils::Convert2RGBA(PerPixel(coord));
			//m_ImageData[i] = Walnut::Random::UInt();
			////m_FinalImageData[i] = 0xffff00ff;//ABGR
			//m_ImageData[i] |= 0xff000000;//使alpha始终为255
		}
	}

	m_FinalImage->SetData(m_ImageData);
}

glm::vec4 Renderer::PerPixel(glm::vec2 coord)
{
	glm::vec3 rayOrigin(0.0f, 0.0f, 2.0f);
	glm::vec3 rayDirection(coord.x, coord.y, -1.0f);
	glm::vec3 sphereOrigin(0.0f);

	glm::vec3 lightDirection = glm::normalize(glm::vec3(1.0f, -1.0f, -0.5f));
	float radius = 0.5f;
	// y = a + bt 代入圆方程
	//(bx^2 + by^2)t^2 + (2(axbx + ayby))t + (ax^2 + ay^2 - r^2) = 0;
	//a = origin
	//b = direction
	//r = radius
	//t = hit distance
	//At^2 + Bt + C
	float A = glm::dot(rayDirection, rayDirection);
	float B = 2.0f * glm::dot(rayOrigin, rayDirection);
	float C = glm::dot(rayOrigin, rayOrigin) - radius * radius;
	//B^2 - 4AC
	float discriminant = B * B - 4.0 * A * C;

	if (discriminant >= 0.0)//有解，与球体相交
	{
		float t0 = (-B - sqrt(discriminant)) / (2.0f * A);
		float t1 = (-B + sqrt(discriminant)) / (2.0f * A);

		glm::vec3 hitPos0 = rayOrigin + rayDirection * t0;
		glm::vec3 hitPos1 = rayOrigin + rayDirection * t1;

		glm::vec3 normal0 = hitPos0 - sphereOrigin;
		glm::vec3 normal1 = hitPos1 - sphereOrigin;

		normal0 = glm::normalize(normal0);
		normal1 = glm::normalize(normal1);

		float cos = glm::max(glm::dot(normal0, -lightDirection), 0.0f);//光照

		//float r = (normal0.x + 1.0) * 0.5 * (cos + 1.0) * 0.5;
		//float g = (normal0.y + 1.0) * 0.5 * (cos + 1.0) * 0.5;
		//float b = (normal0.z + 1.0) * 0.5 * (cos + 1.0) * 0.5;

		glm::vec4 color((normal0 + 1.0f)* 0.5f * cos, 1.0f);
		return color;
	}

	//背景	
	float t_ = 0.5 * (rayDirection.y + 1.0f);//y越大，越蓝
	glm::vec4 color(((1.0f - t_) * glm::vec3(1.0f) + t_ * glm::vec3(0.5, 0.7, 1.0)), 1.0f);

	return color;
}

