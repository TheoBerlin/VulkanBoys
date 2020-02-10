#include "Camera.h"

#include <glm/gtx/euler_angles.hpp>
#include <glm/gtc/matrix_transform.hpp>

const glm::vec3 UP_VECTOR		= glm::vec3(0.0f, 1.0f, 0.0f);
const glm::vec3 FORWARD_VECTOR	= glm::vec3(0.0f, 0.0f, 1.0f);

Camera::Camera()
	: m_Projection(1.0f),
	m_ProjectionInv(1.0f),
	m_View(1.0f),
	m_ViewInv(1.0f),
	m_Position(0.0f),
	m_Rotation(0.0f),
	m_Direction(0.0f),
	m_Right(0.0f),
	m_Up(0.0f),
	m_IsDirty(true)
{
}

void Camera::setDirection(const glm::vec3& direction)
{
	m_Direction = glm::normalize(direction);
	calculateVectors();

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
	//Create rotation matrix
	glm::mat3 rotationMat = glm::eulerAngleYXZ(glm::radians(rotation.y), glm::radians(rotation.x), glm::radians(rotation.z));
	m_Direction	= glm::normalize(rotationMat * FORWARD_VECTOR);
	m_Rotation	= rotation;

	calculateVectors();

	m_IsDirty = true;
}

void Camera::translate(const glm::vec3& translation)
{
	m_Position += (m_Right * translation.x) + (m_Up * translation.y) + (m_Direction * translation.z);
	m_IsDirty = true;
}

void Camera::update()
{
	if (m_IsDirty)
	{
		//Update view
		m_View		= glm::lookAt(m_Position, m_Position + m_Direction, m_Up);
		m_ViewInv	= glm::inverse(m_View);

		m_IsDirty = false;
	}
}

void Camera::calculateVectors()
{
	m_Right = glm::normalize(glm::cross(m_Direction, UP_VECTOR));
	m_Up	= glm::normalize(glm::cross(m_Right, m_Direction));
}
