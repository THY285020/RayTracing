#pragma once
#include <vector>
#include <glm/glm.hpp>

struct Sphere
{
	glm::vec3 Position{ 1.0f };
	float radius = 0.5f;
	glm::vec4 Albedo{ 1.0f };
};

struct Scene
{
	std::vector<Sphere> Spheres;
};