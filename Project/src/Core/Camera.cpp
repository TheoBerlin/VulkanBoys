#include "Camera.h"

#include <glm/gtc/matrix_transform.hpp>

const glm::vec3 UP_VECTOR		= glm::vec3(0.0f, 1.0f, 0.0f);
const glm::vec3 FORWARD_VECTOR	= glm::vec3(0.0f, 0.0f, 1.0f);

Camera::Camera()
	: m_Projection(1.0f),
	m_ProjectionInv(1.0f),
	m_View(1.0f),
	m_ViewInv(1.0f),
	m_Position(0.0f),
	m_Direction(0.0f),
	m_Right(0.0f),
	m_Up(0.0f),
	m_IsDirty(true)
{
}

void Camera::setDirection(const glm::vec3& direction)
{
	m_Direction = glm::normalize(direction);
	m_IsDirty = true;
}

void Camera::setPosition(const glm::vec3& position)
{
	m_Position = position;
	m_IsDirty = true;
}

void Camera::setProjection(float fovDegrees, float width, float height, float nearPlane, float farPlane)
{
	m_Projection	= glm::perspective(glm::radians(fovDegrees), width / height, nearPlane, farPlane);
	m_ProjectionInv = glm::inverse(m_Projection);
}

void Camera::setRotation(const glm::vec3& rotation)
{
	glm::vec3 direction;
	direction.x = cos(glm::radians(rotation.y)) * cos(glm::radians(rotation.x));
	direction.y = sin(glm::radians(rotation.z));
	direction.z = sin(glm::radians(rotation.y)) * cos(glm::radians(rotation.x));

	m_Direction = direction;
	m_IsDirty = true;
}

void Camera::translate(const glm::vec3& translation)
{
	m_Position += translation;
	m_IsDirty = true;
}

void Camera::update()
{
	if (m_IsDirty)
	{
		m_Right = glm::normalize(glm::cross(m_Direction, UP_VECTOR));
		m_Up	= glm::normalize(glm::cross(m_Right, m_Direction));

		//Update view
		m_View	= glm::lookAt(m_Position, m_Position + m_Direction, m_Up);
		m_ViewInv = glm::inverse(m_View);

		m_IsDirty = false;
	}
}
