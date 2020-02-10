#pragma once

#include "Common/IGraphicsContext.h"
#include "VulkanCommon.h"

#include "InstanceVK.h"
#include "DeviceVK.h"

class IWindow;
class SwapChainVK;

class GraphicsContextVK : public IGraphicsContext
{
public:
	DECL_NO_COPY(GraphicsContextVK);

	GraphicsContextVK(IWindow* pWindow);
	~GraphicsContextVK();

	void init();

	//OVERRIDE
	virtual IRenderer* createRenderer() override;
	virtual IImgui* createImgui() override;
    
    virtual IMesh* createMesh() override;

	virtual IShader* createShader() override;
	
	virtual IBuffer* createBuffer() override;
	virtual IFrameBuffer* createFrameBuffer() override;
	
	virtual IImage* createImage() override;
	virtual IImageView* createImageView() override;
	virtual ITexture2D* createTexture2D() override;

	void swapBuffers(VkSemaphore renderSemaphore);

	DeviceVK* getDevice() { return &m_Device; } //Const function?
	SwapChainVK* getSwapChain() const { return m_pSwapChain; }

private:
	IWindow* m_pWindow;
	SwapChainVK* m_pSwapChain;
	InstanceVK m_Instance;
	DeviceVK m_Device;
};
