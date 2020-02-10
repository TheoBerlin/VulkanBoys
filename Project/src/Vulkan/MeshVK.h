#pragma once
#include "Common/IMesh.h"

class BufferVK;
class DeviceVK;

class MeshVK : public IMesh
{
public:
	MeshVK(DeviceVK* pDevice);
	~MeshVK();

	virtual bool initFromFile(const std::string& filepath) override;
	virtual bool initFromMemory(const Vertex* pVertices, uint32_t vertexCount, const uint32_t* pIndices, uint32_t indexCount) override;

	virtual IBuffer* getVertexBuffer() const override;
	virtual IBuffer* getIndexBuffer() const override;

	virtual uint32_t getIndexCount() const override;
	virtual uint32_t getVertexCount() const override;

private:
	DeviceVK* m_pDevice;
	BufferVK* m_pVertexBuffer;
	BufferVK* m_pIndexBuffer;
	uint32_t m_VertexCount;
	uint32_t m_IndexCount;
};