#include "ProfilerVK.h"

#include "vulkan/vulkan.h"

#include "Core/Core.h"
#include "Vulkan/CommandBufferVK.h"
#include "Vulkan/DeviceVK.h"

ProfilerVK::ProfilerVK(const std::string& name, DeviceVK* pDevice, ProfilerVK* pParentProfiler)
    :m_Name(name),
    m_pParent(pParentProfiler),
    m_CurrentFrame(0),
    m_NextQuery(0)
{
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        m_ppQueryPools[i] = nullptr;
    }
}

ProfilerVK::~ProfilerVK()
{
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        SAFEDELETE(m_ppQueryPools[i]);
    }

    for (ProfilerVK* pSubStage : m_ChildProfilers) {
        SAFEDELETE(pSubStage);
    }
}

void ProfilerVK::init(CommandBufferVK* m_ppCommandBuffers[])
{
    m_ppProfiledCommandBuffers = m_ppCommandBuffers;
}

void ProfilerVK::beginFrame(size_t currentFrame)
{
    QueryPoolVK* pCurrentQueryPool = m_ppQueryPools[currentFrame];
    VkCommandBuffer currentCmdBuffer = m_ppProfiledCommandBuffers[currentFrame]->getCommandBuffer();

    vkCmdResetQueryPool(currentCmdBuffer, pCurrentQueryPool->getQueryPool(), 0, pCurrentQueryPool->getQueryCount());
    m_NextQuery = 0;

    // Write a timestamp to measure the time elapsed for the entire scope of the profiler
    vkCmdWriteTimestamp(currentCmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, pCurrentQueryPool->getQueryPool(), m_NextQuery++);

    for (Timestamp* pTimestamp : m_Timestamps) {
        pTimestamp->queries.clear();
        pTimestamp->time = 0.0f;
    }
}

void ProfilerVK::endFrame()
{
    VkQueryPool currentQueryPool = m_ppQueryPools[m_CurrentFrame]->getQueryPool();
    vkCmdWriteTimestamp(m_ppProfiledCommandBuffers[m_CurrentFrame]->getCommandBuffer(), VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, currentQueryPool, m_NextQuery++);

    if (m_NextQuery > m_Timestamps.size()) {
        m_Timestamps.resize(m_NextQuery);
    }

    if (vkGetQueryPoolResults(
        m_pDevice->getDevice(), currentQueryPool,
        0, m_NextQuery,                             // First query, query count
        m_NextQuery * sizeof(uint64_t),             // Data size
        (void*)m_Timestamps.data(),                 // Data pointer
        sizeof(uint64_t),                           // Strides
        VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT)
        != VK_SUCCESS)
    {
        LOG("Profiler %s: failed to get query pool results", m_Name.c_str());
        return;
    }

    writeResults();
}

ProfilerVK* ProfilerVK::createChildProfiler(const std::string& name)
{
    ProfilerVK* pNewStage = DBG_NEW ProfilerVK(name, m_pDevice);
    m_ChildProfilers.push_back(pNewStage);
    return pNewStage;
}

void ProfilerVK::initTimestamp(Timestamp* pTimestamp, const std::string name)
{
    pTimestamp->name = name;
    pTimestamp->time = 0.0f;
    pTimestamp->queries.clear();
    m_Timestamps.push_back(pTimestamp);
}

void ProfilerVK::writeTimestamp(Timestamp* pTimestamp)
{
    pTimestamp->queries.push_back(m_NextQuery);
    vkCmdWriteTimestamp(m_ppProfiledCommandBuffers[m_CurrentFrame]->getCommandBuffer(), VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, m_ppQueryPools[m_CurrentFrame]->getQueryPool(), m_NextQuery++);
}

void ProfilerVK::writeResults()
{
    // Write the profiler's time first
    m_Time = m_TimeResults.back() - m_TimeResults.front();

    for (Timestamp* pTimestamp : m_Timestamps) {
        const std::vector<uint32_t>& timestampQueries = pTimestamp->queries;

        for (size_t queryIdx = 0; queryIdx < timestampQueries.size(); queryIdx += 2) {
            pTimestamp->time += timestampQueries[queryIdx + 1] - timestampQueries[queryIdx];
        }
    }
}
