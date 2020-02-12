#pragma once
#include "Core/Core.h"

#include <glm/glm.hpp>

class IImgui;
class IMesh;
class Camera;

class IRenderer
{
public:
	DECL_INTERFACE(IRenderer);

	virtual bool init() = 0;

	virtual void beginFrame(const Camera& camera) = 0;
	virtual void endFrame() = 0;

	virtual void setClearColor(float r, float g, float b) = 0;
	virtual void setClearColor(const glm::vec3& color) = 0;
	virtual void setViewport(float width, float height, float minDepth, float maxDepth, float topX, float topY) = 0;

	virtual void submitMesh(IMesh* pMesh, const glm::vec4& color, const glm::mat4& transform) = 0;

	virtual void drawImgui(IImgui* pImgui) = 0;

	//Temporary function
	virtual void drawTriangle(const glm::vec4& color, const glm::mat4& transform) = 0;
};
