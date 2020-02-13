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

	ITexture2D* getAlbedoMap() const { return m_pAlbedo; }
	ITexture2D* getNormalMap() const { return m_pNormal; }
	ISampler* getSampler() const { return m_pSampler; };

private:
	//Resources should not be owned by the material?
	ITexture2D* m_pAlbedo;
	ITexture2D* m_pNormal;
	ISampler* m_pSampler;
};