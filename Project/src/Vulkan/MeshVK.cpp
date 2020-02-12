#include "MeshVK.h"
#include "BufferVK.h"
#include "DeviceVK.h"
#include "CopyHandlerVK.h"
#include "CommandPoolVK.h"
#include "CommandBufferVK.h"

#include <tinyobjloader/tiny_obj_loader.h>

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
	tinyobj::attrib_t attributes;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	if (!tinyobj::LoadObj(&attributes, &shapes, &materials, &warn, &err, filepath.c_str(), nullptr, true, false)) 
	{
		LOG("Failed to load mesh '%s'. Warning: %s Error: %s", filepath.c_str(), warn.c_str(), err.c_str());
		return false;
	}

	std::vector<Vertex> vertices = {};
	std::vector<uint32_t> indices = {};
	std::unordered_map<Vertex, uint32_t> uniqueVertices = {};

	for (const tinyobj::shape_t& shape : shapes) 
	{
		for (const tinyobj::index_t& index : shape.mesh.indices) 
		{
			Vertex vertex = {};
			vertex.Position = 
			{
				attributes.vertices[3 * index.vertex_index + 0],
				attributes.vertices[3 * index.vertex_index + 1],
				attributes.vertices[3 * index.vertex_index + 2]
			};

			if (index.normal_index >= 0)
			{
				vertex.Normal =
				{
					attributes.normals[3 * index.normal_index + 0],
					attributes.normals[3 * index.normal_index + 1],
					attributes.normals[3 * index.normal_index + 2]
				};
			}

			vertex.TexCoord = 
			{
				attributes.texcoords[2 * index.texcoord_index + 0],
				1.0f - attributes.texcoords[2 * index.texcoord_index + 1]
			};

			if (uniqueVertices.count(vertex) == 0) 
			{
				uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
				vertices.push_back(vertex);
			}
			 
			indices.push_back(uniqueVertices[vertex]);
		}
	}

	//TODO: Calculate normals

	LOG("-- LOADED MESH: %s", filepath.c_str());
	return initFromMemory(vertices.data(), uint32_t(vertices.size()), indices.data(), uint32_t(indices.size()));
}

bool MeshVK::initFromMemory(const Vertex* pVertices, uint32_t vertexCount, const uint32_t* pIndices, uint32_t indexCount)
{
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

	CopyHandlerVK* pCopyHandler = m_pDevice->getCopyHandler();
	pCopyHandler->updateBuffer(m_pVertexBuffer, 0, pVertices, vertexBufferParams.SizeInBytes);
	pCopyHandler->updateBuffer(m_pIndexBuffer, 0, pIndices, indexBufferParams.SizeInBytes);

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
