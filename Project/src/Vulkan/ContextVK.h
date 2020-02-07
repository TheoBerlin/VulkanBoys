#pragma once

#include "Common/IContext.h"
#include "VulkanCommon.h"

#include "InstanceVK.h"
#include "DeviceVK.h"

class ContextVK : public IContext
{
public:
	DECL_NO_COPY(ContextVK);

	ContextVK();
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

	DeviceVK* getDevice() { return &m_Device; }
private:
	InstanceVK m_Instance;
	DeviceVK m_Device;
};