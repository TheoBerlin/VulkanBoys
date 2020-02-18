#pragma once
#include "Core/Core.h"

class ITexture2D
{
public:
	DECL_INTERFACE(ITexture2D);

	virtual bool initFromFile(const std::string& filename, bool generateMips = true) = 0;
	virtual bool initFromMemory(const void* pData, uint32_t width, uint32_t height, bool generateMips = false) = 0;
};