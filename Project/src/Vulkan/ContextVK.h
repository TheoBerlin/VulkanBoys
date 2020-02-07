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
	IRenderer* createRenderer() override;
	IShader* createShader() override;
	
	IBuffer* createBuffer() override;
	IFrameBuffer* createFrameBuffer() override;
	
	IImage* createImage() override;
	IImageView* createImageView() override;
	ITexture2D* createTexture2D() override;

	DeviceVK* getDevice() const { return &m_Device; }
	SwapChainVK* getSwapChain() const { return m_pSwapChain; }

private:
	IWindow* m_pWindow;
	SwapChainVK* m_pSwapChain;
	InstanceVK m_Instance;
	DeviceVK m_Device;
};