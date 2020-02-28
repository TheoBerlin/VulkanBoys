#pragma once
#include "Common/IShader.h"
#include "VulkanCommon.h"

#include <vector>

class DeviceVK;

class ShaderVK : public IShader
{
public:
	ShaderVK(DeviceVK* pDevice);
	~ShaderVK();

	virtual bool initFromFile(EShader shaderType, const std::string& entrypoint, const std::string& filepath) override;
	virtual bool initFromByteCode(EShader shaderType, const std::string& entrypoint, const std::vector<char>& byteCode) override;
	virtual bool finalize() override;

	virtual EShader getShaderType() const override;
	virtual const std::string& getEntryPoint() const override;

	template <typename T> void setSpecializationConstant(uint32_t index, T data);
	void setSpecializationConstant(uint32_t index, void* pData, uint32_t sizeInBytes);

	const VkSpecializationInfo* getSpecializationInfo() const { return m_SpecializationInfo.dataSize > 0 ? &m_SpecializationInfo : nullptr; }
	VkShaderModule getShaderModule() const { return m_ShaderModule; }

private:
	std::string m_EntryPoint;
	std::vector<char> m_Source;
	DeviceVK* m_pDevice;
	VkShaderModule m_ShaderModule;
	EShader m_ShaderType;

	VkSpecializationInfo m_SpecializationInfo;
	std::vector<VkSpecializationMapEntry> m_SpecializationEntries;
	std::vector<uint8_t> m_SpecializationData;

};

template<typename T>
inline void ShaderVK::setSpecializationConstant(uint32_t index, T data)
{
	setSpecializationConstant(index, &data, sizeof(T));
}
