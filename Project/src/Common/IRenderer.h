#pragma once
#include "Core/Core.h"

#include <glm/glm.hpp>

class IImgui;
class ITextureCube;
class ITexture2D;
class IMesh;
class Camera;
class Material;
class LightSetup;
class GBufferVK;

#define MAX_POINTLIGHTS 4

class IRenderer
{
public:
	DECL_INTERFACE(IRenderer);

	virtual bool init() = 0;

	virtual void beginFrame(const Camera& camera, const LightSetup& lightSetup) = 0;
	virtual void endFrame() = 0;

	virtual void setViewport(float width, float height, float minDepth, float maxDepth, float topX, float topY) = 0;
};
