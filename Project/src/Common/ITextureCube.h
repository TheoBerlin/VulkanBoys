#pragma once
#include "Core/Core.h"

class ITextureCube
{
public:
	DECL_INTERFACE(ITextureCube);

	virtual bool init(uint32_t width, uint32_t miplevels, ETextureFormat format) = 0;
};