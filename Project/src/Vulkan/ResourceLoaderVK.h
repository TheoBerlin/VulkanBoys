#pragma once
#include "Common/IResourceLoader.h"

#include "VulkanCommon.h"

class CommandPoolVK;
class CommandBufferVK;
class GraphicsContextVK;

class ResourceLoaderVK : public IResourceLoader
{
public:
	ResourceLoaderVK(GraphicsContextVK* pContext);
	~ResourceLoaderVK();

	virtual bool init() override;

	virtual IMesh* loadMesh(const std::string& filepath) override;
	virtual ITexture2D* loadTexture(const std::string& filepath) override;

	virtual void waitForResources() override;

private:
	GraphicsContextVK* m_pContext;
	CommandPoolVK* m_pCommandPool;
	CommandBufferVK* m_pCommandBuffer;
};