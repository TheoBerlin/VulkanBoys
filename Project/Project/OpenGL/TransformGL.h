#pragma once
#include "../Transform.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
class TransformGL : public Transform
{
public:
	TransformGL();
	~TransformGL();
	void translate(float x, float y, float z);
	void rotate(float radians, float x, float y, float z);
	glm::mat4 transform;
};

