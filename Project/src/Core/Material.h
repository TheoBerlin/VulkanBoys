#pragma once
#include "Core.h"

class ISampler;	
class ITexture2D;
class IGraphicsContext;

struct SamplerParams;

class Material
{
public:
	Material();
	~Material();

	DECL_NO_COPY(Material);

	bool createSampler(IGraphicsContext* pContext, const SamplerParams& params);

	bool hasAlbedoMap() const;
	bool hasNormalMap() const;
	bool hasAmbientOcclusionMap() const;
	bool hasRoughnessMap() const;
	bool hasMetallicMap() const;

	void setMetallic(float metallic);
	void setRoughness(float roughness);
	void setAmbientOcclusion(float ao);
	void setAlbedo(const glm::vec4& albedo);
	void setAlbedoMap(ITexture2D* pAlbedo);
	void setNormalMap(ITexture2D* pNormal);
	void setAmbientOcclusionMap(ITexture2D* pAO);
	void setMetallicMap(ITexture2D* pMetallic);
	void setRoughnessMap(ITexture2D* pRoughness);

	void release();

	ITexture2D* getAlbedoMap() const { return m_pAlbedoMap; }
	ITexture2D* getNormalMap() const { return m_pNormalMap; }
	ITexture2D* getAmbientOcclusionMap() const { return m_pAmbientOcclusionMap; }
	ITexture2D* getMetallicMap() const { return m_pMetallicMap; }
	ITexture2D* getRoughnessMap() const { return m_pRoughnessMap; }
	ISampler* getSampler() const { return m_pSampler; };
	const glm::vec4& getAlbedo() const { return m_Albedo; }
	uint32_t getMaterialID() const { return m_ID; }
	float getAmbientOcclusion() const { return m_Ambient; }
	float getRoughness() const { return m_Roughness; }
	float getMetallic() const { return m_Metallic; }

private:
	//Resources should not be owned by the material?
	ITexture2D* m_pAlbedoMap;
	ITexture2D* m_pNormalMap;
	ITexture2D* m_pAmbientOcclusionMap;
	ITexture2D* m_pMetallicMap;
	ITexture2D* m_pRoughnessMap;
	ISampler* m_pSampler;
	glm::vec4 m_Albedo;
	float m_Metallic;
	float m_Roughness;
	float m_Ambient;
	const uint32_t m_ID;

	static uint32_t s_ID;
};