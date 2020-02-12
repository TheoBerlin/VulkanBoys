#pragma once

#include "Core/Camera.h"

#include <thread>

class IGraphicsContext;
class IImgui;
class IRenderer;

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

    // Setting a renderer to nullptr will disable it
    virtual void setMeshRenderer(IRenderer* pMeshRenderer) = 0;
    virtual void setRaytracer(IRenderer* pRaytracer) = 0;
    virtual void setParticleRenderer(IRenderer* pParticleRenderer) = 0;

protected:
    IGraphicsContext* m_pGraphicsContext;
    IRenderer* m_pMeshRenderer, *m_pRaytracer, *m_pParticleRenderer;
};
