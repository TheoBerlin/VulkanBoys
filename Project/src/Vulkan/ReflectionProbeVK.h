#pragma once
#include "VulkanCommon.h"

class DeviceVK;
class ImageViewVK;
class RenderPassVK;
class FrameBufferVK;
class TextureCubeVK;

class ReflectionProbeVK
{
public:
	ReflectionProbeVK(DeviceVK* pDevice);
	~ReflectionProbeVK();

	DECL_NO_COPY(ReflectionProbeVK);

	bool initFromTextureCube(TextureCubeVK* pCubemap, RenderPassVK* pRenderPass);

	FrameBufferVK* getFrameBuffer(uint32_t face) const { return m_pFrameBuffers[face]; }

private:
	DeviceVK* m_pDevice;
	FrameBufferVK* m_pFrameBuffers[6]; //6 faces in a cube
	ImageViewVK* m_ppImageViews[6];
};