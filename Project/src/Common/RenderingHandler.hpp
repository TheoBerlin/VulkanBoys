#pragma once
#include "Core/Core.h"

#include <thread>

class IMesh;
class IImgui;
class IScene;
class IRenderer;
class ITexture2D;
class ITextureCube;
class IGraphicsContext;
class ParticleEmitterHandler;

class Camera;
class Material;
class LightSetup;

class RenderingHandler
{
public:
    RenderingHandler();
    virtual ~RenderingHandler();

    DECL_NO_COPY(RenderingHandler);

    virtual bool initialize() = 0;

    virtual ITextureCube* generateTextureCube(ITexture2D* pPanorama, ETextureFormat format, uint32_t width, uint32_t miplevels) = 0;

	virtual void render(IScene* pScene) = 0;

    virtual void drawProfilerUI() = 0;

    virtual void swapBuffers() = 0;

    virtual void setClearColor(float r, float g, float b) = 0;
	virtual void setClearColor(const glm::vec3& color) = 0;
    virtual void setViewport(float width, float height, float minDepth, float maxDepth, float topX, float topY) = 0;
    virtual void setSkybox(ITextureCube* pSkybox) = 0;

    // Setting a renderer to nullptr will disable it
    virtual void setMeshRenderer(IRenderer* pMeshRenderer) = 0;
    virtual void setRayTracer(IRenderer* pRayTracer) = 0;
    virtual void setParticleRenderer(IRenderer* pParticleRenderer) = 0;
    virtual void setVolumetricLightRenderer(IRenderer* pVolumetricLightRenderer) = 0;
    virtual void setImguiRenderer(IImgui* pImGui) = 0;
    virtual IImgui* getImguiRenderer() = 0;

    virtual void onWindowResize(uint32_t width, uint32_t height) = 0;
	virtual void onSceneUpdated(IScene* pScene) = 0;

    void setParticleEmitterHandler(ParticleEmitterHandler* pParticleEmitterHandler) { m_pParticleEmitterHandler = pParticleEmitterHandler; }

protected:
    IGraphicsContext* m_pGraphicsContext;
    ParticleEmitterHandler* m_pParticleEmitterHandler;
};
