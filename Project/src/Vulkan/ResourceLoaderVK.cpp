#include "ResourceLoaderVK.h"
#include "CommandPoolVK.h"
#include "GraphicsContextVK.h"
#include "DeviceVK.h"

ResourceLoaderVK::ResourceLoaderVK(GraphicsContextVK* pContext)
	: m_pContext(pContext),
	m_pCommandPool(nullptr)
{
}

ResourceLoaderVK::~ResourceLoaderVK()
{
	SAFEDELETE(m_pCommandPool);
	m_pContext = nullptr;
}

bool ResourceLoaderVK::init()
{
	DeviceVK* pDevice = m_pContext->getDevice();
	m_pCommandPool = DBG_NEW CommandPoolVK(pDevice, pDevice->getQueueFamilyIndices().graphicsFamily.value());
	if (!m_pCommandPool->init())
	{
		return false;
	}

	m_pCommandBuffer = m_pCommandPool->allocateCommandBuffer();
	if (!m_pCommandBuffer)
	{
		return false;
	}

	return true;
}

IMesh* ResourceLoaderVK::loadMesh(const std::string& filepath)
{
	return nullptr;
}

ITexture2D* ResourceLoaderVK::loadTexture(const std::string& filepath)
{
	return nullptr;
}

void ResourceLoaderVK::waitForResources()
{
}

