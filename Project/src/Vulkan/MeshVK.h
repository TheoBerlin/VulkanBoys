#pragma once
#include "Common/IMesh.h"

#include <map>

class BufferVK;
class DeviceVK;

class MeshVK : public IMesh
{
	struct Triangle
	{
		uint32_t indices[3];
	};

public:
	MeshVK(DeviceVK* pDevice);
	~MeshVK();

	virtual bool initFromFile(const std::string& filepath) override;
	virtual bool initFromMemory(const void* pVertices, size_t vertexSize, uint32_t vertexCount, const uint32_t* pIndices, uint32_t indexCount) override;
	virtual bool initAsSphere(uint32_t subDivisions);

	virtual IBuffer* getVertexBuffer() const override;
	virtual IBuffer* getIndexBuffer() const override;

	virtual uint32_t getIndexCount() const override;
	virtual uint32_t getVertexCount() const override;

	virtual uint32_t getMeshID() const override;

private:
	glm::vec3 calculateTangent(const Vertex& v0, const Vertex& v1, const Vertex& v2);

	uint32_t vertexForEdge(std::map<std::pair<uint32_t, uint32_t>, uint32_t>& lookup, std::vector<glm::vec3>& vertices, uint32_t first, uint32_t second);
	std::vector<Triangle> subdivide(std::vector<glm::vec3>& vertices, std::vector<Triangle>& triangles);

private:
	DeviceVK* m_pDevice;
	BufferVK* m_pVertexBuffer;
	BufferVK* m_pIndexBuffer;
	uint32_t m_VertexCount;
	uint32_t m_IndexCount;
	const uint32_t m_ID;

	static uint32_t s_ID;
};
