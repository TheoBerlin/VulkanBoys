#pragma once
#include "Core.h"

class PointLight
{
public:
	PointLight(const glm::vec3& position = glm::vec3(0.0f), const glm::vec4& color = glm::vec4(1.0f));
	~PointLight() = default;

	void setColor(const glm::vec4& color);
	void setPosition(const glm::vec3& position);

	const glm::vec4& getColor() const { return m_Color; }
	//Return a new vec3 
	glm::vec3 getPosition() const { return m_Position; }

private:
	glm::vec4 m_Color;
	glm::vec4 m_Position;
};