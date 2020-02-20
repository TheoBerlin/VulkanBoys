#pragma once

#include "Core/Core.h"

class IBuffer;
class IFrameBuffer;
class IImgui;
class IImage;
class IImageView;
class IMesh;
class IParticleEmitterHandler;
class IRenderer;
class IRenderingHandler;
class IShader;
class ITexture2D;
class IWindow;
class IResourceLoader;

enum API
{
	VULKAN
};

class IGraphicsContext
{
public:
	DECL_INTERFACE(IGraphicsContext);

	virtual IRenderingHandler* createRenderingHandler() = 0;
	virtual IRenderer* createMeshRenderer(IRenderingHandler* pRenderingHandler) = 0;
	virtual IRenderer* createParticleRenderer(IRenderingHandler* pRenderingHandler) = 0;
	virtual IParticleEmitterHandler* createParticleEmitterHandler() = 0;
	virtual IImgui* createImgui() = 0;

	virtual IShader* createShader() = 0;
	
    virtual IMesh* createMesh() = 0;
    
	virtual IBuffer* createBuffer() = 0;
	virtual void updateBuffer(IBuffer* pDestination, uint64_t destinationOffset, const void* pSource, uint64_t sizeInBytes) = 0;
	virtual IFrameBuffer* createFrameBuffer() = 0;

	virtual IImage* createImage() = 0;
	virtual IImageView* createImageView() = 0;
	virtual ITexture2D* createTexture2D() = 0;

	virtual void sync() = 0;

	virtual bool supportsRayTracing() const = 0;
	
public:
	static IGraphicsContext* create(IWindow* pWindow, API api);
};
