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
	void release();

	ITexture2D* getAlbedoMap() const { return m_pAlbedo; }
	ISampler* getSampler() const { return m_pSampler; };

private:
	//Resources should not be owned by the material?
	ITexture2D* m_pAlbedo;
	ISampler* m_pSampler;
};