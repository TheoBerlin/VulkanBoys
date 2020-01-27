#include "TransformGL.h"

TransformGL::TransformGL()
{
};


TransformGL::~TransformGL()
{
};

void TransformGL::translate(float x, float y, float z)
{
	transform[3][0] += x;
	transform[3][1] += y;
	transform[3][2] += z;
};

void TransformGL::rotate(float radians, float x, float y, float z)
{
	transform = glm::rotate(transform, radians, glm::vec3(x, y, z));
}

