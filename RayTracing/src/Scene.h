#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Utils
{
	constexpr float PI = 3.1415926535;

}

struct Material
{
	glm::vec3 Albedo{ 1.0f };
	float Roughness = 1.0f;
	float Metallic = 0.0f;
	glm::vec3 EmissionColor{ 0.0f };
	float EmissionPower = 0.0f;

	glm::vec3 GetEmission()const { return EmissionColor * EmissionPower; }
};

struct Object
{
	glm::vec3 Position;
	int MaterialIndex;
	virtual glm::vec3 GetPosition()const { return Position; }
	virtual int GetMaterialIndex()const { return MaterialIndex; }
};

struct Sphere : Object
{
	glm::vec3 Position{ 1.0f };
	float Radius = 0.5f;
	int MaterialIndex = 0;
	virtual glm::vec3 GetPosition()const override { return Position; }
	virtual int GetMaterialIndex()const override { return MaterialIndex; }
};

struct Quad : Object
{
	glm::vec3 Position{ 0.0f };
	glm::vec3 Rotation{ 0.0f };
	float Width = 1.0f;
	float Height = 1.0f;
	int MaterialIndex = 0;

	glm::vec4 x = { 1,0,0,1 };
	glm::vec4 y = { 0,1,0,1 };
	glm::vec4 z = { 0,0,1,1 };

	glm::mat4 model = glm::mat4(1.0f);
	glm::mat3 transform = glm::mat3(1.0f);

	void Init()
	{
		model = glm::rotate(model, glm::radians(Rotation.x), glm::vec3(1.0, 0.0, 0.0))
		*glm::rotate(model, glm::radians(Rotation.y), glm::vec3(0.0, 1.0, 0.0))
		*glm::rotate(model, glm::radians(Rotation.z), glm::vec3(0.0, 0.0, 1.0));

		x = model * x;
		y = model * y;
		z = model * z;

		transform = glm::transpose(glm::mat3(x, y, z));
	}

	void RotateLocal(glm::vec3& origin, glm::vec3& direction)
	{
		origin = transform * origin  ;
		direction = transform * direction  ;
	}

	glm::vec3 LocalTransform(glm::vec3 OorD)
	{
		return glm::vec3(transform * OorD);
	}

	void RotateRay(glm::vec3& origin, glm::vec3& direction)const 
	{
		//z
		if (!(Rotation.z <= 0.0001f && Rotation.z >= -0.0001f))
		{
			float radians = (Rotation.z / 180.0f) * Utils::PI;
			float cos_theta = cos(radians);
			float sin_theta = sin(radians);
			origin.x =  cos_theta * origin.x + sin_theta * origin.y;
			origin.y = -sin_theta * origin.x + cos_theta * origin.y;
			direction.x =  cos_theta * direction.x + sin_theta * direction.y;
			direction.y = -sin_theta * direction.x + cos_theta * direction.y;
		}
		//x
		if (!(Rotation.x <= 0.0001f && Rotation.x >= -0.0001f))
		{
			float radians = (Rotation.x / 180.0f) * Utils::PI;
			float cos_theta = cos(radians);
			float sin_theta = sin(radians);
			origin.y = cos_theta * origin.y + sin_theta * origin.z;
			origin.z = -sin_theta * origin.y + cos_theta * origin.z;
			direction.y = cos_theta * direction.y + sin_theta * direction.z;
			direction.z = -sin_theta * direction.y + cos_theta * direction.z;
		}
		//y
		if (!(Rotation.y <= 0.0001f && Rotation.y >= -0.0001f))
		{
			float radians = (Rotation.y / 180.0f) * Utils::PI;
			float cos_theta = cos(radians);
			float sin_theta = sin(radians);
			origin.x = cos_theta * origin.x - sin_theta * origin.z;
			origin.z = sin_theta * origin.x + cos_theta * origin.z;
			direction.x = cos_theta * direction.x - sin_theta * direction.z;
			direction.z = sin_theta * direction.x + cos_theta * direction.z;
		}
	}

	void Rotate(glm::vec3& hitPos, glm::vec3& normal)const
	{
		//z
		if (!(Rotation.z <= 0.0001f && Rotation.z >= -0.0001f))
		{
			float radians = (Rotation.z / 180.0f) * Utils::PI;
			float cos_theta = cos(radians);
			float sin_theta = sin(radians);
			hitPos.x = cos_theta * hitPos.x - sin_theta * hitPos.y;
			hitPos.y = sin_theta * hitPos.x + cos_theta * hitPos.y;

			normal.x = cos_theta * normal.x - sin_theta * normal.y;
			normal.y = sin_theta * normal.x + cos_theta * normal.y;
		}

		//x
		if (!(Rotation.x <= 0.0001f && Rotation.x >= -0.0001f))
		{
			float radians = (Rotation.x / 180.0f) * Utils::PI;
			float cos_theta = cos(radians);
			float sin_theta = sin(radians);
			hitPos.y = cos_theta * hitPos.y - sin_theta * hitPos.z;
			hitPos.z = sin_theta * hitPos.y + cos_theta * hitPos.z;

			normal.y = cos_theta * normal.y - sin_theta * normal.z;
			normal.z = sin_theta * normal.y + cos_theta * normal.z;
		}

		//y
		if (!(Rotation.y <= 0.0001f && Rotation.y >= -0.0001f))
		{
			float radians = (Rotation.y / 180.0f) * Utils::PI;
			float cos_theta = cos(radians);
			float sin_theta = sin(radians);
			hitPos.x = cos_theta * hitPos.x + sin_theta * hitPos.z;
			hitPos.z = -sin_theta * hitPos.x + cos_theta * hitPos.z;

			normal.x = cos_theta * normal.x + sin_theta * normal.z;
			normal.z = -sin_theta * normal.x + cos_theta * normal.z;
		}
	}
	virtual glm::vec3 GetPosition()const override { return Position; }
	virtual int GetMaterialIndex()const override { return MaterialIndex; }

};

struct Scene
{
	std::vector<Sphere> Spheres;
	std::vector<Quad> Quads;
	std::vector<Material> Materials;
};