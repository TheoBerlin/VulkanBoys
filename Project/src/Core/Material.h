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

	FORCEINLINE ITexture2D*			getAlbedoMap() const			{ return m_pAlbedoMap; }
	FORCEINLINE ITexture2D*			getNormalMap() const			{ return m_pNormalMap; }
	FORCEINLINE ITexture2D*			getAmbientOcclusionMap() const	{ return m_pAmbientOcclusionMap; }
	FORCEINLINE ITexture2D*			getMetallicMap() const			{ return m_pMetallicMap; }
	FORCEINLINE ITexture2D*			getRoughnessMap() const			{ return m_pRoughnessMap; }
	FORCEINLINE ISampler*			getSampler() const				{ ASSERT(m_pSampler != nullptr); return m_pSampler; };
	FORCEINLINE const glm::vec4&	getAlbedo() const				{ return m_Albedo; }
	FORCEINLINE uint32_t			getMaterialID() const			{ return m_ID; }
	FORCEINLINE float				getAmbientOcclusion() const		{ return m_Ambient; }
	FORCEINLINE float				getRoughness() const			{ return m_Roughness; }
	FORCEINLINE float				getMetallic() const				{ return m_Metallic; }
	FORCEINLINE const glm::vec3&	getMaterialProperties() const	{ return m_MaterialProperties; }

private:
	//Resources should not be owned by the material?
	glm::vec4 m_Albedo;
	union
	{
		struct
		{
			float m_Ambient;
			float m_Metallic;
			float m_Roughness;
		};

		glm::vec3 m_MaterialProperties;
	};

	ITexture2D* m_pAlbedoMap;
	ITexture2D* m_pNormalMap;
	ITexture2D* m_pAmbientOcclusionMap;
	ITexture2D* m_pMetallicMap;
	ITexture2D* m_pRoughnessMap;
	ISampler*	m_pSampler;
	
	const uint32_t m_ID;

	static uint32_t s_ID;
};