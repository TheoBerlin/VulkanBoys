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

	// TODO: Remove these once a ray tracer renderer class exists, and use beginFrame and endFrame instead
	virtual void beginRayTraceFrame(const Camera& camera) = 0;
	virtual void endRayTraceFrame() = 0;
	virtual void traceRays() = 0;

	virtual void setViewport(float width, float height, float minDepth, float maxDepth, float topX, float topY) = 0;

	virtual void submitMesh(IMesh* pMesh, const glm::vec4& color, const glm::mat4& transform) = 0;

	virtual void drawImgui(IImgui* pImgui) = 0;
};
