#pragma once
#include "Common/IShader.h"
#include "Common.h"

#include <vector>

class DeviceVK;

class ShaderVK : public IShader
{
public:
	ShaderVK(DeviceVK* pDevice);
	~ShaderVK();

	virtual bool loadFromFile(EShader shaderType, const std::string& entrypoint, const std::string& filepath) override;
	virtual bool finalize() override;

	virtual EShader getShaderType() const override;
	virtual const std::string& getEntryPoint() const override;

private:
	std::string m_EntryPoint;
	std::vector<char> m_Source;
	DeviceVK* m_pDevice;
	VkShaderModule m_ShaderModule;
	EShader m_ShaderType;
};