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

#define MAX_POINTLIGHTS 13

class IRenderer
{
public:
	DECL_INTERFACE(IRenderer);

	virtual bool init() = 0;

	virtual ITextureCube* generateTextureCubeFromPanorama(ITexture2D* pPanorama, uint32_t width, uint32_t miplevels, ETextureFormat format) = 0;

	virtual void onWindowResize(uint32_t width, uint32_t height) = 0;

	virtual void beginFrame(const Camera& camera, const LightSetup& lightSetup) = 0;
	virtual void endFrame() = 0;

	virtual void setClearColor(float r, float g, float b) = 0;
	virtual void setClearColor(const glm::vec3& color) = 0;
	virtual void setViewport(float width, float height, float minDepth, float maxDepth, float topX, float topY) = 0;

	virtual void swapBuffers() = 0;

	virtual void submitMesh(IMesh* pMesh, const Material& material, const glm::mat4& transform) = 0;
	
	virtual void drawImgui(IImgui* pImgui) = 0;
};
