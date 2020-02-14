#pragma once

#include "Core/Camera.h"

#include <thread>

class IGraphicsContext;
class IImgui;
class IRenderer;
class IMesh;

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

    virtual void drawImgui(IImgui* pImgui) = 0;

    virtual void setClearColor(float r, float g, float b) = 0;
	virtual void setClearColor(const glm::vec3& color) = 0;
    virtual void setViewport(float width, float height, float minDepth, float maxDepth, float topX, float topY) = 0;

    virtual void submitMesh(IMesh* pMesh, const glm::vec4& color, const glm::mat4& transform) = 0;

    // Setting a renderer to nullptr will disable it
    void setMeshRenderer(IRenderer* pMeshRenderer) { m_pMeshRenderer = pMeshRenderer; }
    void setRayTracer(IRenderer* pRayTracer) { m_pRayTracer = pRayTracer; }
    void setParticleRenderer(IRenderer* pParticleRenderer) { m_pParticleRenderer = pParticleRenderer; }

protected:
    IGraphicsContext* m_pGraphicsContext;
    IRenderer* m_pMeshRenderer, *m_pRayTracer, *m_pParticleRenderer;
};
