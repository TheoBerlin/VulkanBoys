#pragma once
#include "Core/Core.h"

#include <glm/glm.hpp>

class IImgui;

class IRenderer
{
public:
	DECL_INTERFACE(IRenderer);

	virtual bool init() = 0;

	virtual void onWindowResize(uint32_t width, uint32_t height) = 0;

	virtual void beginFrame() = 0;
	virtual void endFrame() = 0;

	virtual void setClearColor(float r, float g, float b) = 0;
	virtual void setClearColor(const glm::vec3& color) = 0;
	virtual void setViewport(float width, float height, float minDepth, float maxDepth, float topX, float topY) = 0;

	virtual void swapBuffers() = 0;

	virtual void drawImgui(IImgui* pImgui) = 0;

	//Temporary function
	virtual void drawTriangle(const glm::vec4& color) = 0;
};