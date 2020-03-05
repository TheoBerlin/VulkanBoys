#pragma once
#include "VulkanCommon.h"

#include <vector>

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

	FrameBufferVK* getFrameBuffer(uint32_t face, uint32_t miplevel) const { return m_FrameBuffers[(miplevel * 6U) + face]; }

private:
	DeviceVK* m_pDevice;
	std::vector<ImageViewVK*> m_ImageViews;
	std::vector<FrameBufferVK*> m_FrameBuffers; 
};