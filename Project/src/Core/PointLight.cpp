#include "PointLight.h"

PointLight::PointLight(const glm::vec3& position, const glm::vec4& color)
	: m_Position(position, 0.0f),
	m_Color(color)
{
}

void PointLight::setColor(const glm::vec4& color)
{
	m_Color = color;
}

void PointLight::setPosition(const glm::vec3& position)
{
	m_Position = glm::vec4(position, 0.0f);
}
