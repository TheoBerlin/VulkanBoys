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

	void setAlbedoMap(ITexture2D* pAlbedo);
	void setNormalMap(ITexture2D* pNormal);
	
	void release();

	ITexture2D* getAlbedoMap() const { return m_pAlbedoMap; }
	ITexture2D* getNormalMap() const { return m_pNormalMap; }
	ISampler* getSampler() const { return m_pSampler; };
	uint32_t getMaterialID() const { return m_ID; }

private:
	//Resources should not be owned by the material?
	ITexture2D* m_pAlbedoMap;
	ITexture2D* m_pNormalMap;
	ISampler* m_pSampler;
	const uint32_t m_ID;

	static uint32_t s_ID;
};