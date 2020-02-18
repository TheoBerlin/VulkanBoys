#pragma once
#include "Core/Core.h"

enum class ETextureFormat : uint8_t
{
	FORMAT_NONE					= 0,
	FORMAT_R8G8B8A8_UNORM		= 1,
	FORMAT_R32G32B32A32_FLOAT	= 2
};

class ITexture2D
{
public:
	DECL_INTERFACE(ITexture2D);

	virtual bool initFromFile(const std::string& filename, ETextureFormat format, bool generateMips = true) = 0;
	virtual bool initFromMemory(const void* pData, uint32_t width, uint32_t height, ETextureFormat format, bool generateMips = false) = 0;
};