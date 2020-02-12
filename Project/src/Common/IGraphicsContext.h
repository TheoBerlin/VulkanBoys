#pragma once

#include "Core/Core.h"

class IImgui;
class IImage;
class IWindow;
class IShader;
class IBuffer;
class IRenderer;
class IImageView;
class ITexture2D;
class IFrameBuffer;
class IMesh;
class IResourceLoader;

enum API
{
	VULKAN
};

class IGraphicsContext
{
public:
	DECL_INTERFACE(IGraphicsContext);

	virtual IRenderer* createRenderer() = 0;
	virtual IImgui* createImgui() = 0;

	virtual IShader* createShader() = 0;
	
    virtual IMesh* createMesh() = 0;
    
	virtual IBuffer* createBuffer() = 0;
	virtual IFrameBuffer* createFrameBuffer() = 0;

	virtual IImage* createImage() = 0;
	virtual IImageView* createImageView() = 0;
	virtual ITexture2D* createTexture2D() = 0;

	virtual void sync() = 0;
	
public:
	static IGraphicsContext* create(IWindow* pWindow, API api);
};
