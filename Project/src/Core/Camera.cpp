#include "Camera.h"

Camera::Camera()
	: m_Projection(1.0f),
	m_ProjectionInv(1.0f),
	m_View(1.0f),
	m_ViewInv(1.0f),
	m_IsDirty(true)
{
}

void Camera::setProjection()
{
}

void Camera::setView()
{
}
