#pragma once
#include "Common/IImgui.h"

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

	virtual bool init(uint32_t width, uint32_t height) override;
	virtual void render(CommandBufferVK* pCommandBuffer) override;

	virtual void onWindowClose() override;
	virtual void onWindowResize(uint32_t width, uint32_t height) override;
	virtual void onWindowFocusChanged(IWindow* pWindow, bool hasFocus) override;

private:
	bool initImgui(uint32_t width, uint32_t height);
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
	BufferVK* m_pVertexBuffer;
	BufferVK* m_pIndexBuffer;
};

