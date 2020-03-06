#include "MeshVK.h"
#include "BufferVK.h"
#include "DeviceVK.h"
#include "CopyHandlerVK.h"
#include "CommandPoolVK.h"
#include "CommandBufferVK.h"

#include <tinyobjloader/tiny_obj_loader.h>
#include <array>

uint32_t MeshVK::s_ID = 0;

MeshVK::MeshVK(DeviceVK* pDevice)
	: m_pDevice(pDevice),
	m_pVertexBuffer(nullptr),
	m_pIndexBuffer(nullptr),
	m_IndexCount(0),
	m_VertexCount(0),
	m_ID(s_ID++)
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

			//Normals and texcoords are optional, while positions are required
			ASSERT(index.vertex_index >= 0);
			
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
			
			if (index.texcoord_index >= 0)
			{
				vertex.TexCoord = 
				{
					attributes.texcoords[2 * index.texcoord_index + 0],
					1.0f -	attributes.texcoords[2 * index.texcoord_index + 1]
				};
			}

			if (uniqueVertices.count(vertex) == 0) 
			{
				uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
				vertices.push_back(vertex);
			}
			 
			indices.push_back(uniqueVertices[vertex]);
		}
	}

	//Calculate tangents
	for (uint32_t index = 0; index < indices.size(); index += 3)
	{
		Vertex& v0 = vertices[indices[index + 0]];
		Vertex& v1 = vertices[indices[index + 1]];
		Vertex& v2 = vertices[indices[index + 2]];

		v0.Tangent = calculateTangent(v0, v1, v2);
		v1.Tangent = calculateTangent(v1, v2, v0);
		v2.Tangent = calculateTangent(v2, v0, v1);
	}

	//TODO: Calculate normals

	LOG("-- LOADED MESH: %s", filepath.c_str());
	return initFromMemory(vertices.data(), sizeof(Vertex), uint32_t(vertices.size()), indices.data(), uint32_t(indices.size()));
}

bool MeshVK::initFromMemory(const void* pVertices, size_t vertexSize, uint32_t vertexCount, const uint32_t* pIndices, uint32_t indexCount)
{
	BufferParams vertexBufferParams = {};
	vertexBufferParams.Usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	vertexBufferParams.SizeInBytes = vertexSize * vertexCount;
	vertexBufferParams.MemoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	m_pVertexBuffer = DBG_NEW BufferVK(m_pDevice);
	if (!m_pVertexBuffer->init(vertexBufferParams))
	{
		return false;
	}

	BufferParams indexBufferParams = {};
	indexBufferParams.Usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
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

bool MeshVK::initAsSphere(uint32_t subDivisions)
{
	const float X = 0.525731112119133606f;
	const float Z = 0.850650808352039932f;
	const float N = 0.0f;

	static const std::vector<glm::vec3> icosahedronVertices =
	{
		{-X,N,Z}, {X,N,Z}, {-X,N,-Z}, {X,N,-Z},
		{N,Z,X}, {N,Z,-X}, {N,-Z,X}, {N,-Z,-X},
		{Z,X,N}, {-Z,X, N}, {Z,-X,N}, {-Z,-X, N}
	};

	static const std::vector<Triangle> icosahedronTriangles =
	{
		{0,4,1},{0,9,4},{9,5,4},{4,5,8},{4,8,1},
		{8,10,1},{8,3,10},{5,3,8},{5,2,3},{2,7,3},
		{7,10,3},{7,6,10},{7,11,6},{11,0,6},{0,1,6},
		{6,1,10},{9,0,11},{9,11,2},{9,2,5},{7,2,11}
	};

	std::vector<glm::vec3> vertices = icosahedronVertices;
	std::vector<Triangle> triangles = icosahedronTriangles;

	for (uint32_t i = 0; i < subDivisions; ++i)
	{
		triangles = subdivide(vertices, triangles);
	}

	std::vector<Vertex> finalVertices;
	finalVertices.reserve(vertices.size());
	std::vector<uint32_t> finalIndices;
	finalIndices.reserve(triangles.size() * 3);
	
	for (uint32_t v = 0; v < vertices.size(); v++)
	{
		finalVertices.push_back({ glm::vec4(vertices[v], 1.0f), glm::vec4(vertices[v], 0.0f), glm::vec4(0.0f), glm::vec4(0.0f) });
	}

	for (uint32_t i = 0; i < triangles.size(); i++)
	{
		finalIndices.push_back(triangles[i].indices[0]);
		finalIndices.push_back(triangles[i].indices[1]);
		finalIndices.push_back(triangles[i].indices[2]);
	}

	initFromMemory(finalVertices.data(), sizeof(Vertex), (uint32_t)finalVertices.size(), finalIndices.data(), (uint32_t)finalIndices.size());
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

uint32_t MeshVK::getMeshID() const
{
	return m_ID;
}

glm::vec3 MeshVK::calculateTangent(const Vertex& v0, const Vertex& v1, const Vertex& v2)
{
	glm::vec3 edge1 = v1.Position - v0.Position;
	glm::vec3 edge2 = v2.Position - v0.Position;
	glm::vec2 deltaUV1 = v1.TexCoord - v0.TexCoord;
	glm::vec2 deltaUV2 = v2.TexCoord - v0.TexCoord;

	float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

	glm::vec3 tangent;
	tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
	tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
	tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
	tangent = glm::normalize(tangent);

	return tangent;
}

uint32_t MeshVK::vertexForEdge(std::map<std::pair<uint32_t, uint32_t>, uint32_t>& lookup, std::vector<glm::vec3>& vertices, uint32_t first, uint32_t second)
{
	std::map<std::pair<uint32_t, uint32_t>, uint32_t>::key_type key(first, second);
	if (key.first > key.second)
		std::swap(key.first, key.second);

	auto inserted = lookup.insert({ key, vertices.size() });
	if (inserted.second)
	{
		auto& edge0 = vertices[first];
		auto& edge1 = vertices[second];
		auto point = glm::normalize(edge0 + edge1);
		vertices.push_back(point);
	}

	return inserted.first->second;
}

std::vector<MeshVK::Triangle> MeshVK::subdivide(std::vector<glm::vec3>& vertices, std::vector<Triangle>& triangles)
{
	std::map<std::pair<uint32_t, uint32_t>, uint32_t> lookup;
	std::vector<Triangle> result;

	for (auto&& each : triangles)
	{
		std::array<uint32_t, 3> mid;
		for (int edge = 0; edge < 3; ++edge)
		{
			mid[edge] = vertexForEdge(lookup, vertices, each.indices[edge], each.indices[(edge + 1) % 3]);
		}

		result.push_back({ each.indices[0], mid[0], mid[2] });
		result.push_back({ each.indices[1], mid[1], mid[0] });
		result.push_back({ each.indices[2], mid[2], mid[1] });
		result.push_back({ mid[0], mid[1], mid[2] });
	}

	return result;
}
