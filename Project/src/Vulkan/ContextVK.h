#pragma once

#include "Common/IContext.h"
#include "VulkanCommon.h"

#include "InstanceVK.h"
#include "DeviceVK.h"

class IWindow;
class SwapChainVK;

class ContextVK : public IContext
{
public:
	DECL_NO_COPY(ContextVK);

	ContextVK(IWindow* pWindow);
	~ContextVK();

	void init();

	//OVERRIDE
	virtual IRenderer* createRenderer() override;
	virtual IShader* createShader() override;
	
	virtual IBuffer* createBuffer() override;
	virtual IFrameBuffer* createFrameBuffer() override;
	
	virtual IImage* createImage() override;
	virtual IImageView* createImageView() override;
	virtual ITexture2D* createTexture2D() override;

	virtual void swapBuffers() override;

	DeviceVK* getDevice() { return &m_Device; } //Const function?
	SwapChainVK* getSwapChain() const { return m_pSwapChain; }

private:
	IWindow* m_pWindow;
	SwapChainVK* m_pSwapChain;
	InstanceVK m_Instance;
	DeviceVK m_Device;
};