#pragma once
#include "VulkanCommon.h"

class ImageVK;
class DeviceVK;
class ImageViewVK;
class RenderPassVK;
class FrameBufferVK;

class GBufferVK
{
public:
	GBufferVK(DeviceVK* pDevice);
	~GBufferVK();

	DECL_NO_COPY(GBufferVK);

	void resize(uint32_t width, uint32_t height);

	void addColorAttachmentFormat(VkFormat format);
	void setDepthAttachmentFormat(VkFormat format);
	bool finalize(RenderPassVK* pRenderPass, uint32_t width, uint32_t height);

	FORCEINLINE ImageVK*				getDepthImage() const						{ return m_pDepthImage; }
	FORCEINLINE ImageViewVK*			getDepthImageView() const					{ return m_pDepthImageView; }
	FORCEINLINE ImageVK* const *		getColorImages() const						{ return m_ColorImages.data(); }
	FORCEINLINE ImageViewVK* const *	getColorImageViews() const					{ return m_ColorImageViews.data(); }
	FORCEINLINE uint32_t				getColorImageCount() const					{ return uint32_t(m_ColorImages.size()); }
	FORCEINLINE ImageViewVK*			getDepthAttachment() const					{ return m_pDepthImageView; }
	FORCEINLINE ImageViewVK*			getColorAttachment(uint32_t index) const	{ return m_ColorImageViews[index]; }
	FORCEINLINE FrameBufferVK*			getFrameBuffer() const						{ return m_pFrameBuffer; }
	FORCEINLINE VkExtent2D				getExtent() const							{ return m_Extent; }

private:
	void releaseBuffers();
	bool createImages();
	bool createImageViews();
	bool createFrameBuffer(RenderPassVK* pRenderPass);

private:
	DeviceVK* m_pDevice;
	FrameBufferVK* m_pFrameBuffer;
	RenderPassVK* m_pRenderPass;
	ImageVK* m_pDepthImage;
	ImageViewVK* m_pDepthImageView;
	std::vector<VkFormat> m_ColorFormats;
	std::vector<ImageVK*> m_ColorImages;
	std::vector<ImageViewVK*> m_ColorImageViews;
	VkFormat m_DepthFormat;
	VkExtent2D m_Extent;
};

