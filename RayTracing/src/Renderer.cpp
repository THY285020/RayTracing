#include "Renderer.h"
#include "Walnut/Random.h"
#include <algorithm>
#include <imgui_internal.h>
#include <execution>

//PerPixel->TraceRay(循环多次计算bounce)->ClosetHit/Miss
//像素-》逆变换（proj 透视除法 view）求出虚拟光线方向-》计算虚拟光线碰撞-》返回颜色到像素缓存-》输出
//roughness 通过随机偏移法线
//accumulate 求多帧平均值解决噪音
//std::for_each(std::execution::par, iterator.begin(), iterator.end(), func);多线程优化循环
namespace Utils
{
	static uint32_t Convert2RGBA(const glm::vec4& color)
	{
		uint8_t r = (uint8_t)(color.r * 255.0f);//映射到0-255
		uint8_t g = (uint8_t)(color.g * 255.0f);
		uint8_t b = (uint8_t)(color.b * 255.0f);
		uint8_t a = (uint8_t)(color.a * 255.0f);

		return (a << 24) | (b << 16) | (g << 8) | r;//ABGR 16进制=2^4占4位 高位存在低地址（大端存储）
	}

	static float PCH_Hash(uint32_t input)
	{
		uint32_t state = input * 747796405u + 2891336453u;
		uint32_t word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
		return (word >> 22u) ^ word;
	}

	static float RandomFloat(uint32_t& seed)//生成0-1的随机数
	{
		seed = PCH_Hash(seed);
		return (float)seed / (float)std::numeric_limits<uint32_t>::max();
	}

	static float RandomNormalDistribution(uint32_t& seed)
	{
		float theta = 2 * (Utils::PI * RandomFloat(seed));
		float rho = sqrt(-2 * log(Utils::PI * RandomFloat(seed)));
		return rho * cos(theta);
	}

	static glm::vec3 InUnitSphere(uint32_t& seed)
	{
		return glm::normalize(glm::vec3(
			RandomFloat(seed) * 2.0f - 1.0f,
			RandomFloat(seed) * 2.0f - 1.0f,
			RandomFloat(seed) * 2.0f - 1.0f

			//RandomNormalDistribution(seed), //转-1 1
			//RandomNormalDistribution(seed),
			//RandomNormalDistribution(seed)
		)
		);
	}

	static glm::vec3 InUnitHemiSphere(uint32_t& seed, glm::vec3 normal)
	{
		glm::vec3 dir = InUnitSphere(seed);
		return dir * float(signed(dot(normal, dir)));
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

	m_ImageData = std::make_shared<uint32_t[]>(width * height);

	//delete[] m_ImageData;
	//m_ImageData = new uint32_t[width * height];

	m_AccumulationData = std::make_shared<glm::vec4[]>(width * height);

	//delete[] m_AccumulationData;
	//m_AccumulationData = new glm::vec4[width * height];

	m_HorizontalIterator.resize(width);
	m_VerticalIterator.resize(height);
	for (uint32_t i = 0; i < width; ++i)
		m_HorizontalIterator[i] = i;
	for (uint32_t i = 0; i < height; ++i)
		m_VerticalIterator[i] = i;

	ResetFrameIndex();
}

//static glm::vec3 lightDir = glm::vec3(-1.0f, -1.0f, -1.0f);

void Renderer::Render(Scene& scene, Camera& camera)
{
	m_ActiveScene = &scene;
	m_ActiveCamera = &camera;

	if (m_FrameIndex == 1)
		memset(m_AccumulationData.get(), 0, m_FinalImage->GetWidth() * m_FinalImage->GetHeight() * sizeof(glm::vec4));
	//light
	//ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	//ImGui::Begin("LightDir");
	//std::string label = "LightDir";
	//ImGui::PushID(label.c_str());
	//ImGui::Columns(2);
	//ImGui::SetColumnWidth(0, 80.f);
	//ImGui::Text(label.c_str());
	//ImGui::NextColumn();

	//ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
	//ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.f, 0.f));

	//float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
	//ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

	//ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f,1.0f });
	//ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f,1.0f });
	//ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f,1.0f });

	//if (ImGui::Button("X", buttonSize))
	//	lightDir.x = 0.f;

	//ImGui::PopStyleColor(3);
	//ImGui::SameLine();
	//ImGui::DragFloat("##X", &lightDir.x, 0.1, 0.0, 0.0, "%.2f");
	//ImGui::PopItemWidth();
	//ImGui::SameLine();

	//ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.7f, 0.15f,1.0f });
	//ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.8f, 0.2f,1.0f });
	//ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.7f, 0.15f,1.0f });

	//if (ImGui::Button("Y", buttonSize))
	//	lightDir.y = 0.f;

	//ImGui::PopStyleColor(3);
	//ImGui::SameLine();
	//ImGui::DragFloat("##Y", &lightDir.y, 0.1, 0.0, 0.0, "%.2f");
	//ImGui::PopItemWidth();
	//ImGui::SameLine();

	//ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.2f, 0.8f, 1.0f });
	//ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.3f, 0.9f,1.0f });
	//ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.2f, 0.8f, 1.0f });

	//if (ImGui::Button("Z", buttonSize))
	//	lightDir.z = 0.f;

	//ImGui::PopStyleColor(3);
	//ImGui::SameLine();
	//ImGui::DragFloat("##Z", &lightDir.z, 0.1, 0.0, 0.0, "%.2f");
	//ImGui::PopItemWidth();

	//ImGui::PopStyleVar();
	//ImGui::Columns(1);
	//ImGui::PopID();
	//ImGui::End();
	//ImGui::PopStyleVar();
	//light

#define MT 1
#ifdef MT
	//for_each会把每个迭代器的对象输入lambda函数
	std::for_each(std::execution::par, m_VerticalIterator.begin(), m_VerticalIterator.end(),
		[this](uint32_t y)
		{
#if 0
			std::for_each(std::execution::par, m_HorizontalIterator.begin(), m_HorizontalIterator.end(),
				[this, y](uint32_t x)
				{
					glm::vec4 color = PerPixel(x, y);

					//求叠加的平均颜色以此降低噪声
					m_AccumulationData[x + y * m_FinalImage->GetWidth()] += color;
					glm::vec4 accumulateColor = m_AccumulationData[x + y * m_FinalImage->GetWidth()];
					accumulateColor /= m_FrameIndex;

					accumulateColor = glm::clamp(accumulateColor, glm::vec4(0.0f), glm::vec4(1.0f));//防止颜色大于1
					//屏幕上每一个坐标对应一束虚拟光，通过虚拟光计算颜色
					m_ImageData[x + y * m_FinalImage->GetWidth()] = utils::Convert2RGBA(accumulateColor);
				}
			);
#endif
			for (uint32_t x = 0; x < m_FinalImage->GetWidth(); ++x)
			{
				glm::vec4 color = PerPixel(x, y);

				//求叠加的平均颜色以此降低噪声
				m_AccumulationData[x + y * m_FinalImage->GetWidth()] += color;
				glm::vec4 accumulateColor = m_AccumulationData[x + y * m_FinalImage->GetWidth()];
				accumulateColor /= m_FrameIndex;

				accumulateColor = glm::clamp(accumulateColor, glm::vec4(0.0f), glm::vec4(1.0f));//防止颜色大于1
				//屏幕上每一个坐标对应一束虚拟光，通过虚拟光计算颜色
				m_ImageData[x + y * m_FinalImage->GetWidth()] = Utils::Convert2RGBA(accumulateColor);
			}
		}
	);
#else
	//更新缓冲
	for (uint32_t y = 0; y < m_FinalImage->GetHeight(); ++y)
	{
		for (uint32_t x = 0; x < m_FinalImage->GetWidth(); ++x)
		{
			glm::vec4 color = PerPixel(x, y);

			//求叠加的平均颜色以此降低噪声
			m_AccumulationData[x + y * m_FinalImage->GetWidth()] += color;
			glm::vec4 accumulateColor = m_AccumulationData[x + y * m_FinalImage->GetWidth()];
			accumulateColor /= m_FrameIndex;

			accumulateColor = glm::clamp(accumulateColor, glm::vec4(0.0f), glm::vec4(1.0f));//防止颜色大于1
			//屏幕上每一个坐标对应一束虚拟光，通过虚拟光计算颜色
			m_ImageData[x + y* m_FinalImage->GetWidth()] = Utils::Convert2RGBA(accumulateColor);
			//m_ImageData[i] = Walnut::Random::UInt();
			////m_FinalImageData[i] = 0xffff00ff;//ABGR
			//m_ImageData[i] |= 0xff000000;//使alpha始终为255
		}
	}
#endif
	m_FinalImage->SetData(m_ImageData.get());

	if (m_Settings.Accumulate)
		m_FrameIndex++;
	else
		m_FrameIndex = 1;
}

glm::vec4 Renderer::PerPixel(uint32_t x, uint32_t y)
{
	Ray ray;
	ray.Origin = m_ActiveCamera->GetPosition();
	ray.Direction = m_ActiveCamera->GetRayDirections()[y * m_FinalImage->GetWidth() + x];

	glm::vec3 light(0.0f);
	glm::vec3 contribution(1.0f);

	uint32_t seed = x + y * m_FinalImage->GetWidth();
	seed *= m_FrameIndex;

	int bounces = 5;
	for (int i = 0; i < bounces; ++i)
	{
		Renderer::HitPayload payload = TraceRay(ray);
		if (payload.HitDistance < 0.0f)
		{
			glm::vec3 skyColor = glm::vec3(0.6f, 0.7f, 0.9f);
			light += skyColor * contribution;
			break;
		}

		//glm::vec3 lightDir(-1.0f);
		//const Sphere& Hitsphere = m_ActiveScene->Spheres[payload.ObjectIndex];
		const Material& material = m_ActiveScene->Materials[payload.Object->GetMaterialIndex()];

		//float cos = glm::max(glm::dot(payload.WorldNormal, -glm::normalize(lightDir)), 0.0f);//光照强度
		//glm::vec3 sphereColor(material.Albedo);
		//light += (sphereColor * contribution);

		float cos = glm::dot(payload.WorldNormal, glm::normalize(-ray.Direction));
		float pdf = 0.0f;
		if (cos > 0)
			pdf = cos / Utils::PI;

		contribution *= material.Albedo * pdf;//接近法线的地方占比大
		light += material.GetEmission() + contribution;

		ray.Origin = payload.WorldPosition + payload.WorldNormal * 0.0001f;//更新光线的出发点
		//ray.Direction = glm::reflect(ray.Direction, payload.WorldNormal + material.Roughness * Walnut::Random::Vec3(-0.5, 0.5));

		if(m_Settings.SlowRandom)
			ray.Direction = glm::normalize(payload.WorldNormal + Walnut::Random::InUnitSphere());
		else
		{
			seed += i;
			//ray.Direction = glm::normalize(payload.WorldNormal + Utils::InUnitHemiSphere(seed, payload.WorldNormal));
			ray.Direction = glm::normalize(payload.WorldNormal + Utils::InUnitSphere(seed));
		}
	}

	return glm::vec4(light, 1.0f);
}

Renderer::HitPayload Renderer::TraceRay(const Ray& ray)
{
	if (m_ActiveScene->Spheres.size() == 0) return Miss(ray);
	
	std::shared_ptr<Object> object;
	//int closestSphereIndex = -1;
	//int closestQuadIndex = -1;
	bool miss = true;
	float hitDistance = FLT_MAX;
	Primitive hitC = Primitive::Sphere;

	for (size_t i = 0; i < m_ActiveScene->Spheres.size(); ++i)
	{
		const Sphere& sphere = m_ActiveScene->Spheres[i];
		glm::vec3 spherehitPos = sphere.Position;
		glm::vec3 hitPos = ray.Origin - spherehitPos;//实际是虚拟光线反向移动，计算交点时使用相机与物体的相对位置

		// y = a + bt 代入圆方程
		//(bx^2 + by^2)t^2 + (2(axbx + ayby))t + (ax^2 + ay^2 - r^2) = 0;
		//a = hitPos
		//b = direction
		//r = radius
		//t = hit distance
		//At^2 + Bt + C
		float A = glm::dot(ray.Direction, ray.Direction);
		float B = 2.0f * glm::dot(hitPos, ray.Direction);
		float C = glm::dot(hitPos, hitPos) - sphere.Radius * sphere.Radius;
		//B^2 - 4AC
		float discriminant = B * B - 4.0 * A * C;

		if (discriminant >= 0.0)//有解，与球体相交
		{
			float t0 = (-B - glm::sqrt(discriminant)) / (2.0f * A);
			//float t1 = (-B + sqrt(discriminant)) / (2.0f * A);

			if (t0 > 0.0f && t0 < hitDistance)//确保大于0防止，位置移动到镜头却渲染背面
			{
				hitDistance = t0;
				miss = false;
				object = std::make_shared<Sphere>(sphere);
				hitC = Primitive::Sphere;
			}
		}
	}

	for (size_t i = 0; i < m_ActiveScene->Quads.size(); ++i)
	{
		Quad& quad = const_cast<Quad&>(m_ActiveScene->Quads[i]);//new
		
		//const Quad& quad = m_ActiveScene->Quads[i];

		quad.Init();

		glm::vec3 origin = ray.Origin;
		glm::vec3 direction = ray.Direction;
		//quad.RotateRay(origin, direction);
		origin = quad.LocalTransform(origin);
		direction = quad.LocalTransform(direction);
		glm::vec3 Position = quad.LocalTransform(quad.Position);
		//quad.RotateLocal(origin, direction);//new

		//rotate y
		//float angle = -30.0f;
		//float radians = -(30.0f / 180.0f) * Utils::PI;
		//float cos_theta = cos(radians);
		//float sin_theta = sin(radians);
		//
		//origin.x = cos_theta * ray.Origin.x - sin_theta * ray.Origin.z;
		//origin.z = sin_theta * ray.Origin.x + cos_theta * ray.Origin.z;
		//direction.x = cos_theta * ray.Direction.x - sin_theta * ray.Direction.z;
		//direction.z = sin_theta * ray.Direction.x + cos_theta * ray.Direction.z;

		//float t = (quad.Position.z - origin.z)/ direction.z;//new
		//float t = (quad.Position.z - origin.z) / direction.z;
		float t = (Position.z - origin.z) / direction.z;
		float x = origin.x + t * direction.x;
		float y = origin.y + t * direction.y;

		float x1 = Position.x + 0.5 * quad.Width;		 //0.5f * quad.Width;//
		float x0 = Position.x - 0.5 * quad.Width;	//-0.5f * quad.Width;/
		float y1 = Position.y + 0.5 * quad.Height;	 //0.5f * quad.Height;/
		float y0 = Position.y - 0.5 * quad.Height;	 //-0.5f * quad.Height;

		//float x1 =  0.5f * quad.Width;
		//float x0 =  -0.5f * quad.Width;
		//float y1 = 	0.5f * quad.Height;
		//float y0 = 	-0.5f * quad.Height;
		if (x > x1 || x < x0 || y > y1 || y < y0 || t < 0)
			continue;
		if (t < hitDistance)
		{
			hitDistance = t;
			miss = false;
			object = std::make_shared<Quad>(quad);
			hitC = Primitive::Quad;
		}
	}

	//if (closestSphereIndex < 0 && closestQuadIndex < 0)
	if(miss)
		return Miss(ray);

	return ClosestHit(ray, hitDistance, hitC, object);

	//原背景色	
	//float t_ = 0.5 * (ray.Direction.y + 1.0f);//y越大，越蓝
	//glm::vec4 color(((1.0f - t_) * glm::vec3(1.0f) + t_ * glm::vec3(0.5, 0.7, 1.0)), 1.0f);
}

//碰撞点的计算法线和世界坐标
Renderer::HitPayload Renderer::ClosestHit(const Ray& ray, float hitDistance, Primitive hitPrimitive, std::shared_ptr<Object> object)
{
	Renderer::HitPayload payload;
	payload.HitDistance = hitDistance;
	//payload.ObjectIndex = objectIndex;
	payload.Object = object;

	if (hitPrimitive == Primitive::Sphere)
	{
		//const Sphere& closestSphere = m_ActiveScene->Spheres[objectIndex];

		//glm::vec3 hitPos = ray.hitPos - closestSphere.Position;//转换为球体局部坐标，相机与物体的相对位置
		glm::vec3 hitPos = ray.Origin - object->GetPosition();//转换为球体局部坐标，相机与物体的相对位置

		glm::vec3 hitPos0 = hitPos + ray.Direction * hitDistance;//从球心-方向
		//glm::vec3 hitPos1 = ray.hitPos + ray.Direction * t1;
		
		payload.WorldNormal = glm::normalize(hitPos0);
		//glm::vec3 normal1 = hitPos1 - hitPos;
		//payload.WorldPosition = hitPos0 + closestSphere.Position;//上面计算减了，要加回去
		payload.WorldPosition = hitPos0 + object->GetPosition();//上面计算减了，要加回去
	}
	else if(hitPrimitive == Primitive::Quad)
	{
		//const Quad& closestQuad = m_ActiveScene->Quads[objectIndex];
		glm::vec3 hitPos = ray.Origin + ray.Direction * hitDistance;

		glm::vec3 normal = glm::vec3(0.0, 0.0, 1.0);

		std::dynamic_pointer_cast<Quad>(object)->Rotate(hitPos, normal);

		normal = dot(ray.Direction, normal) < 0 ? normal : -normal;

		payload.WorldNormal = normal;
		//payload.WorldPosition = closestQuad.Position;
		payload.WorldPosition = hitPos;
	}

	return payload;
}

Renderer::HitPayload Renderer::Miss(const Ray& ray)
{
	Renderer::HitPayload payload;
	payload.HitDistance = -1.0f;
	return payload;
}

//-----------------



