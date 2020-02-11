#pragma once
#include "Core/Core.h"

class IMesh;
class ITexture2D;

class IResourceLoader
{
public:
	DECL_INTERFACE(IResourceLoader);

	virtual bool init() = 0;

	virtual IMesh* loadMesh(const std::string& filepath) = 0;
	virtual ITexture2D* loadTexture(const std::string& filepath) = 0;

	virtual void waitForResources() = 0;
};