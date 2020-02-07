#include "DescriptorPoolHandlerVK.h"

#include "DescriptorSetLayoutVK.h"

#include <iostream>

DescriptorPoolHandlerVK::DescriptorPoolHandlerVK()
    :m_pDevice(nullptr),
    m_CurrentFrame(0)
{}

DescriptorPoolHandlerVK::~DescriptorPoolHandlerVK()
{}

bool DescriptorPoolHandlerVK::allocDescriptorSet(DescriptorSetVK* pDescriptorSet, const DescriptorSetLayoutVK* pDescriptorSetLayout)
{
    std::vector<DescriptorPoolVK>& framePool = m_DescriptorPools[m_CurrentFrame];

    // Calculate the requested amount of descriptor allocations
    DescriptorCounts allocationRequest = pDescriptorSetLayout->getBindingCounts();

    for (DescriptorPoolVK descriptorPool : framePool) {
        if (descriptorPool.hasRoomFor(allocationRequest)) {
            if (descriptorPool.allocDescriptorSet(pDescriptorSet, pDescriptorSetLayout)) {
                return true;
            }
        }
    }

    // No existing descriptor pool had room for the descriptor set, make a new one
    constexpr uint32_t setsPerPool = 10;
    allocationRequest.enlarge(setsPerPool);

    DescriptorPoolVK descriptorPool;
    descriptorPool.initializeDescriptorPool(m_pDevice, allocationRequest, setsPerPool);
    bool success = descriptorPool.allocDescriptorSet(pDescriptorSet, pDescriptorSetLayout);
    framePool.push_back(DescriptorPoolVK());

    return success;
}
