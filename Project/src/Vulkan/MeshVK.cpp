#include "MeshVK.h"
#include "BufferVK.h"
#include "DeviceVK.h"
#include "CommandPoolVK.h"
#include "CommandBufferVK.h"

MeshVK::MeshVK(DeviceVK* pDevice)
	: m_pDevice(pDevice),
	m_pVertexBuffer(nullptr),
	m_pIndexBuffer(nullptr),
	m_IndexCount(0),
	m_VertexCount(0)
{
}

MeshVK::~MeshVK()
{
	SAFEDELETE(m_pVertexBuffer);
	SAFEDELETE(m_pIndexBuffer);

	m_pDevice = 0;
}

bool MeshVK::initFromFile(const std::string& filepath)
{
	return false;
}

bool MeshVK::initFromMemory(const Vertex* pVertices, uint32_t vertexCount, const uint32_t* pIndices, uint32_t indexCount)
{
	CommandPoolVK* pCommandPool = DBG_NEW CommandPoolVK(m_pDevice, m_pDevice->getQueueFamilyIndices().transferFamily.value());
	pCommandPool->init();

	CommandBufferVK* pCommandBuffer = pCommandPool->allocateCommandBuffer();

	BufferParams vertexBufferParams = {};
	vertexBufferParams.Usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	vertexBufferParams.SizeInBytes = sizeof(Vertex) * vertexCount;
	vertexBufferParams.MemoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	m_pVertexBuffer = DBG_NEW BufferVK(m_pDevice);
	if (!m_pVertexBuffer->init(vertexBufferParams))
	{
		return false;
	}

	BufferParams indexBufferParams = {};
	indexBufferParams.Usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	indexBufferParams.SizeInBytes = sizeof(uint32_t) * indexCount;
	indexBufferParams.MemoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	m_pIndexBuffer = DBG_NEW BufferVK(m_pDevice);
	if (!m_pIndexBuffer->init(indexBufferParams))
	{
		return false;
	}

	pCommandBuffer->reset();
	pCommandBuffer->begin();

	pCommandBuffer->updateBuffer(m_pVertexBuffer, 0, pVertices, vertexBufferParams.SizeInBytes);
	pCommandBuffer->updateBuffer(m_pIndexBuffer, 0, pIndices, indexBufferParams.SizeInBytes);

	pCommandBuffer->end();

	m_pDevice->executeCommandBuffer(m_pDevice->getTransferQueue(), pCommandBuffer, nullptr, nullptr, 0, nullptr, 0);
	m_pDevice->wait();

	SAFEDELETE(pCommandPool);

	m_VertexCount	= vertexCount;
	m_IndexCount	= indexCount;
	return true;
}

IBuffer* MeshVK::getVertexBuffer() const
{
	return m_pVertexBuffer;
}

IBuffer* MeshVK::getIndexBuffer() const
{
	return m_pIndexBuffer;
}

uint32_t MeshVK::getIndexCount() const
{
	return m_IndexCount;
}

uint32_t MeshVK::getVertexCount() const
{
	return m_VertexCount;
}
