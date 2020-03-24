#pragma once
#include "Core/Core.h"

class IMesh;
class IImgui;
class IImage;
class IScene;
class IBuffer;
class IShader;
class IWindow;
class ISampler;
class IRenderer;
class IImageView;
class ITexture2D;
class IFrameBuffer;
class IFrameBuffer;
class IResourceLoader;
class RenderingHandler;
class ParticleEmitterHandler;

enum API
{
	VULKAN
};

class IGraphicsContext
{
public:
	DECL_INTERFACE(IGraphicsContext);

	virtual RenderingHandler* createRenderingHandler() = 0;
	virtual IRenderer* createMeshRenderer(RenderingHandler* pRenderingHandler) = 0;
	virtual IRenderer* createParticleRenderer(RenderingHandler* pRenderingHandler) = 0;
	virtual IRenderer* createRayTracingRenderer(RenderingHandler* pRenderingHandler) = 0;
	virtual ParticleEmitterHandler* createParticleEmitterHandler() = 0;
	virtual IImgui* createImgui() = 0;

	virtual IScene* createScene(const RenderingHandler* pRenderingHandler) = 0;

	virtual IShader* createShader() = 0;

    virtual IMesh* createMesh() = 0;

	virtual IBuffer* createBuffer() = 0;
	virtual void updateBuffer(IBuffer* pDestination, uint64_t destinationOffset, const void* pSource, uint64_t sizeInBytes) = 0;
	virtual IFrameBuffer* createFrameBuffer() = 0;

	virtual IImage* createImage() = 0;
	virtual IImageView* createImageView() = 0;
	virtual ITexture2D* createTexture2D() = 0;
	virtual ISampler* createSampler() = 0;

	virtual void sync() = 0;

	virtual bool setRayTracingEnabled(bool enabled) = 0;
	virtual bool isRayTracingEnabled() const = 0;

public:
	static IGraphicsContext* create(IWindow* pWindow, API api);
};
