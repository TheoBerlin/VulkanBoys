#pragma once
#include "Core.h"

class ITexture2D;

class Material
{
public:
	Material();
	~Material();

	DECL_NO_COPY(Material);

	void setAlbedo(ITexture2D* pAlbedo);
	void release();

private:
};