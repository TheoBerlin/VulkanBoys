#pragma once
#include <glm/glm.hpp>

struct CameraBuffer
{
	glm::mat4 Projection	= glm::mat4(1.0f);
	glm::mat4 View			= glm::mat4(1.0f);
};

class Camera
{
public:
	Camera();
	~Camera() = default;

	void setProjection();
	void setView();

	const glm::mat4& getProjectionMat() const { return m_Projection; }
	const glm::mat4& getProjectionInvMat() const { return m_ProjectionInv; }
	const glm::mat4& getViewMat() const { return m_View; }
	const glm::mat4& getViewInvMat() const { return m_ViewInv; }

private:
	glm::mat4 m_Projection;
	glm::mat4 m_ProjectionInv;
	glm::mat4 m_View;
	glm::mat4 m_ViewInv;
	bool m_IsDirty;
};

