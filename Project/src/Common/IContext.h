#pragma once

#include "Core/Core.h"

class IImage;
class IWindow;
class IShader;
class IBuffer;
class IRenderer;
class IImageView;
class ITexture2D;
class IFrameBuffer;

enum API
{
	VULKAN
};

class IContext
{
public:
	DECL_INTERFACE(IContext);

	virtual IRenderer* createRenderer() = 0;
	
	virtual IShader* createShader() = 0;
	
	virtual IBuffer* createBuffer() = 0;
	virtual IFrameBuffer* createFrameBuffer() = 0;

	virtual IImage* createImage() = 0;
	virtual IImageView* createImageView() = 0;
	virtual ITexture2D* createTexture2D() = 0;

	virtual void swapBuffers() = 0;
	
public:
	static IContext* create(IWindow* pWindow, API api);
};