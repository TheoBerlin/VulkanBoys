#pragma once
#include "Core/Core.h"

#include <vector>

enum class EShader : uint32_t
{
	NONE			= 0,
	VERTEX_SHADER	= (1 << 0),
	GEOMETRY_SHADER	= (1 << 1),
	HULL_SHADER		= (1 << 2),
	DOMAIN_SHADER	= (1 << 3),
	PIXEL_SHADER	= (1 << 4),
	COMPUTE_SHADER	= (1 << 5)
};

class IShader
{
public:
	DECL_INTERFACE(IShader);

	virtual bool initFromFile(EShader shaderType, const std::string& entrypoint, const std::string& filepath) = 0;
	virtual bool initFromByteCode(EShader shaderType, const std::string& entrypoint, const std::vector<char>& byteCode) = 0;
	virtual bool finalize() = 0;

	virtual EShader getShaderType() const = 0;
	virtual const std::string& getEntryPoint() const = 0;
};