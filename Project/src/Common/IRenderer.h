#pragma once
#include "Core/Core.h"

#include <glm/glm.hpp>

class Camera;
class GBufferVK;
class IImgui;
class IScene;
class ITextureCube;
class ITexture2D;
class IMesh;
class LightSetup;
class Material;
class Profiler;

#define MAX_POINTLIGHTS 4

class IRenderer
{
public:
	DECL_INTERFACE(IRenderer);

	virtual bool init() = 0;

	virtual void beginFrame(IScene* pScene) = 0;
	virtual void endFrame(IScene* pScene) = 0;

	virtual void renderUI() = 0;

	virtual void setViewport(float width, float height, float minDepth, float maxDepth, float topX, float topY) = 0;

	// Gets the sum of the latest frametime from the renderer's profiler(s)
	virtual double getElapsedTime() const { return 0.0; }
};
