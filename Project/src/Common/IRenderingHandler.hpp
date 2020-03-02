#pragma once

#include "Core/Camera.h"

#include <thread>

class IGraphicsContext;
class IImgui;
class IRenderer;
class IMesh;
class IParticleEmitterHandler;

class IRenderingHandler
{
public:
    IRenderingHandler();
    virtual ~IRenderingHandler();

    virtual bool initialize() = 0;

    virtual void onWindowResize(uint32_t width, uint32_t height) = 0;

    virtual void beginFrame(const Camera& camera) = 0;
    virtual void endFrame() = 0;
    virtual void swapBuffers() = 0;

    virtual void drawProfilerUI() = 0;
    virtual void drawImgui(IImgui* pImgui) = 0;

    virtual void setClearColor(float r, float g, float b) = 0;
	virtual void setClearColor(const glm::vec3& color) = 0;
    virtual void setViewport(float width, float height, float minDepth, float maxDepth, float topX, float topY) = 0;

    virtual void submitMesh(IMesh* pMesh, const glm::vec4& color, const glm::mat4& transform) = 0;

    // Setting a renderer to nullptr will disable it
    virtual void setMeshRenderer(IRenderer* pMeshRenderer) = 0;
    virtual void setRayTracer(IRenderer* pRayTracer) = 0;
    virtual void setParticleRenderer(IRenderer* pParticleRenderer) = 0;

    void setParticleEmitterHandler(IParticleEmitterHandler* pParticleEmitterHandler) { m_pParticleEmitterHandler = pParticleEmitterHandler; }

protected:
    IGraphicsContext* m_pGraphicsContext;

    IParticleEmitterHandler* m_pParticleEmitterHandler;
};
