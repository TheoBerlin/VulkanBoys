#pragma once

#include "Common/IParticleEmitterHandler.h"
#include "Common/ITexture2D.h"
#include "Core/ParticleEmitter.h"

class IPipeline;
class IGraphicsContext;
class ITexture2D;
class ParticleEmitter;

class IRenderer;
class Texture2DVK;
struct ParticleStorage;

class ParticleEmitterHandlerVK : public IParticleEmitterHandler
{
public:
    virtual void updateBuffers(IRenderingHandler* pRenderingHandler) override;
};
