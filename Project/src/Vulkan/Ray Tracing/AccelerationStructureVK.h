#pragma once

#include "../VulkanCommon.h"

class BufferVK;

class AccelerationStructureVK
{
public:
	DECL_NO_COPY(AccelerationStructureVK);

	AccelerationStructureVK();
	~AccelerationStructureVK();

	bool addBLAS(BufferVK* pVertexBuffer, BufferVK* pIndexBuffer);
	void finalize();
};
