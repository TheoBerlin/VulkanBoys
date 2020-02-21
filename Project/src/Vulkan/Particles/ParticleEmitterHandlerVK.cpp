#include "ParticleEmitterHandlerVK.h"

#include "Common/Debug.h"
#include "Common/IGraphicsContext.h"
#include "Common/IShader.h"
#include "Vulkan/BufferVK.h"
#include "Vulkan/CommandBufferVK.h"
#include "Vulkan/RenderingHandlerVK.h"

void ParticleEmitterHandlerVK::updateBuffers(IRenderingHandler* pRenderingHandler)
{
    RenderingHandlerVK* pRenderingHandlerVK = reinterpret_cast<RenderingHandlerVK*>(pRenderingHandler);
    CommandBufferVK* pCommandBuffer = pRenderingHandlerVK->getCurrentGraphicsCommandBuffer();

    for (ParticleEmitter* pEmitter : m_ParticleEmitters) {
        BufferVK* pEmitterBuffer = reinterpret_cast<BufferVK*>(pEmitter->getEmitterBuffer());
        BufferVK* pParticleBuffer = reinterpret_cast<BufferVK*>(pEmitter->getParticleBuffer());

        EmitterBuffer emitterBuffer = {};
        emitterBuffer.particleSize = pEmitter->getParticleSize();
        pCommandBuffer->updateBuffer(pEmitterBuffer, 0, &emitterBuffer, sizeof(EmitterBuffer));

        const std::vector<glm::vec4>& particlePositions = pEmitter->getParticleStorage().positions;
        ParticleBuffer particleBuffer = {}; // TODO: How do I insert data into an array within a storage buffer struct?
        particleBuffer.positions = particlePositions.data();
        pCommandBuffer->updateBuffer(pParticleBuffer, 0, particlePositions.data(), sizeof(glm::vec4) * particlePositions.size());
    }
}
