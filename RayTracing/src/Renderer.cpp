#include "Renderer.h"
#include "Walnut/Random.h"
#include <algorithm>
#include <imgui_internal.h>

//PerPixel->TraceRay(循环多次计算bounce)->ClosetHit/Miss
//像素-》逆变换（proj 透视除法 view）求出虚拟光线方向-》计算虚拟光线碰撞-》返回颜色到像素缓存-》输出
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

static glm::vec3 lightDir = glm::vec3(-1.0f, -1.0f, -1.0f);

void Renderer::Render(Scene& scene, Camera& camera)
{
	m_ActiveScene = &scene;
	m_ActiveCamera = &camera;
	//light
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("LightDir");
	std::string label = "LightDir";
	ImGui::PushID(label.c_str());
	ImGui::Columns(2);
	ImGui::SetColumnWidth(0, 80.f);
	ImGui::Text(label.c_str());
	ImGui::NextColumn();

	ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.f, 0.f));

	float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
	ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f,1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f,1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f,1.0f });

	if (ImGui::Button("X", buttonSize))
		lightDir.x = 0.f;

	ImGui::PopStyleColor(3);
	ImGui::SameLine();
	ImGui::DragFloat("##X", &lightDir.x, 0.1, 0.0, 0.0, "%.2f");
	ImGui::PopItemWidth();
	ImGui::SameLine();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.7f, 0.15f,1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.8f, 0.2f,1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.7f, 0.15f,1.0f });

	if (ImGui::Button("Y", buttonSize))
		lightDir.y = 0.f;

	ImGui::PopStyleColor(3);
	ImGui::SameLine();
	ImGui::DragFloat("##Y", &lightDir.y, 0.1, 0.0, 0.0, "%.2f");
	ImGui::PopItemWidth();
	ImGui::SameLine();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.2f, 0.8f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.3f, 0.9f,1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.2f, 0.8f, 1.0f });

	if (ImGui::Button("Z", buttonSize))
		lightDir.z = 0.f;

	ImGui::PopStyleColor(3);
	ImGui::SameLine();
	ImGui::DragFloat("##Z", &lightDir.z, 0.1, 0.0, 0.0, "%.2f");
	ImGui::PopItemWidth();

	ImGui::PopStyleVar();
	ImGui::Columns(1);
	ImGui::PopID();
	ImGui::End();
	ImGui::PopStyleVar();
	//light

	//更新缓冲
	for (uint32_t y = 0; y < m_FinalImage->GetHeight(); ++y)
	{
		for (uint32_t x = 0; x < m_FinalImage->GetWidth(); ++x)
		{
			glm::vec4 color = PerPixel(x, y);
			
			//屏幕上每一个坐标对应一束虚拟光，通过虚拟光计算颜色
			m_ImageData[x + y* m_FinalImage->GetWidth()] = utils::Convert2RGBA(color);
			//m_ImageData[i] = Walnut::Random::UInt();
			////m_FinalImageData[i] = 0xffff00ff;//ABGR
			//m_ImageData[i] |= 0xff000000;//使alpha始终为255
		}
	}

	m_FinalImage->SetData(m_ImageData);
}

glm::vec4 Renderer::PerPixel(uint32_t x, uint32_t y)
{
	Ray ray;
	ray.Origin = m_ActiveCamera->GetPosition();
	ray.Direction = m_ActiveCamera->GetRayDirections()[y * m_FinalImage->GetWidth() + x];

	glm::vec3 color(0.0f);
	float multiplier = 1.0f;

	int bounces = 2;
	for (int i = 0; i < bounces; ++i)
	{
		Renderer::HitPayload payload = TraceRay(ray);
		if (payload.HitDistance < 0)
		{
			glm::vec3 skyColor = glm::vec3(0.0f);
			color += skyColor * multiplier;
			break;
		}

		const Sphere& Hitsphere = m_ActiveScene->Spheres[payload.ObjectIndex];
		float cos = glm::max(glm::dot(payload.WorldNormal, -glm::normalize(lightDir)), 0.0f);//光照强度
		glm::vec3 sphereColor(Hitsphere.Albedo * cos);
		color += (sphereColor * multiplier);

		multiplier *= 0.7f;
	
		ray.Origin = payload.WorldPosition + payload.WorldNormal * 0.001f;
		ray.Direction = glm::reflect(ray.Direction, payload.WorldNormal);
	}

	return glm::vec4(color, 1.0f);
}

Renderer::HitPayload Renderer::TraceRay(const Ray& ray)
{
	if (m_ActiveScene->Spheres.size() == 0) return Miss(ray);

	int closestSphereIndex = -1;
	float hitDistance = FLT_MAX;

	for (size_t i = 0; i < m_ActiveScene->Spheres.size(); ++i)
	{
		const Sphere& sphere = m_ActiveScene->Spheres[i];
		glm::vec3 sphereOrigin = sphere.Position;
		glm::vec3 origin = ray.Origin - sphereOrigin;//实际是虚拟光线反向移动，计算交点时使用相机与物体的相对位置

		// y = a + bt 代入圆方程
		//(bx^2 + by^2)t^2 + (2(axbx + ayby))t + (ax^2 + ay^2 - r^2) = 0;
		//a = origin
		//b = direction
		//r = radius
		//t = hit distance
		//At^2 + Bt + C
		float A = glm::dot(ray.Direction, ray.Direction);
		float B = 2.0f * glm::dot(origin, ray.Direction);
		float C = glm::dot(origin, origin) - sphere.Radius * sphere.Radius;
		//B^2 - 4AC
		float discriminant = B * B - 4.0 * A * C;

		if (discriminant >= 0.0)//有解，与球体相交
		{
			float t0 = (-B - sqrt(discriminant)) / (2.0f * A);
			//float t1 = (-B + sqrt(discriminant)) / (2.0f * A);

			if (t0 > 0.0f && t0 < hitDistance)//确保大于0防止，位置移动到镜头却渲染背面
			{
				hitDistance = t0;
				closestSphereIndex = (int)i;
			}
		}
	}

	if (closestSphereIndex < 0)
		return Miss(ray);

	return ClosetHit(ray, hitDistance, closestSphereIndex);

	//原背景色	
	//float t_ = 0.5 * (ray.Direction.y + 1.0f);//y越大，越蓝
	//glm::vec4 color(((1.0f - t_) * glm::vec3(1.0f) + t_ * glm::vec3(0.5, 0.7, 1.0)), 1.0f);
}

//碰撞点的计算法线和世界坐标
Renderer::HitPayload Renderer::ClosetHit(const Ray& ray, float hitDistance, int objectIndex)
{
	Renderer::HitPayload payload;
	payload.HitDistance = hitDistance;
	payload.ObjectIndex = objectIndex;

	const Sphere& closestSphere = m_ActiveScene->Spheres[objectIndex];

	glm::vec3 origin = ray.Origin - closestSphere.Position;//转换为局部坐标，相机与物体的相对位置

	glm::vec3 hitPos0 = origin + ray.Direction * hitDistance;
	//glm::vec3 hitPos1 = ray.Origin + ray.Direction * t1;

	payload.WorldNormal = glm::normalize(hitPos0);
	//glm::vec3 normal1 = hitPos1 - origin;
	payload.WorldPosition = hitPos0 + closestSphere.Position;//上面计算减了，要加回去

	return payload;
}

Renderer::HitPayload Renderer::Miss(const Ray& ray)
{
	Renderer::HitPayload payload;
	payload.HitDistance = -1.0f;
	return payload;
}



