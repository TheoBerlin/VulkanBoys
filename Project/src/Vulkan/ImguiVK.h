#pragma once
#include "Common/IImgui.h"

#include "VulkanCommon.h"

class IShader;
class BufferVK;
class SamplerVK;
class PipelineVK;
class ITexture2D;
class RenderPassVK;
class DescriptorSetVK;
class DescriptorPoolVK;
class PipelineLayoutVK;
class GraphicsContextVK;
class DescriptorSetLayoutVK;

class ImguiVK : public IImgui
{
public:
	ImguiVK(GraphicsContextVK* pContext);
	~ImguiVK();

	virtual bool init() override;
	
	virtual void begin(double deltatime) override;
	virtual void end() override;
	
	virtual void render(CommandBufferVK* pCommandBuffer, uint32_t currentFrame) override;
	
	virtual void onMouseMove(uint32_t x, uint32_t y) override;
	virtual void onMousePressed(int32_t button) override;
	virtual void onMouseScroll(double x, double y) override;
	virtual void onMouseReleased(int32_t button) override;

	virtual void onKeyTyped(uint32_t character) override;
	virtual void onKeyPressed(EKey key) override;
	virtual void onKeyReleased(EKey key) override;

private:
	bool initImgui();
	bool createRenderPass();
	bool createPipeline();
	bool createPipelineLayout();
	bool createFontTexture();
	bool createBuffers(uint32_t vertexBufferSize, uint32_t indexBufferSize);

private:
	GraphicsContextVK* m_pContext;
	SamplerVK* m_pSampler;
	PipelineVK* m_pPipeline;
	RenderPassVK* m_pRenderPass;
	DescriptorSetVK* m_pDescriptorSet;
	DescriptorPoolVK* m_pDescriptorPool;
	DescriptorSetLayoutVK* m_pDescriptorSetLayout;
	PipelineLayoutVK* m_pPipelineLayout;
	ITexture2D* m_pFontTexture;
	BufferVK* m_ppVertexBuffers[MAX_FRAMES_IN_FLIGHT];
	BufferVK* m_ppIndexBuffers[MAX_FRAMES_IN_FLIGHT];
};
