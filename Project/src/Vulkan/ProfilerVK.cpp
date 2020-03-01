#include "ProfilerVK.h"

#include "imgui/imgui.h"
#include "vulkan/vulkan.h"

#include "Core/Core.h"
#include "Vulkan/CommandBufferVK.h"
#include "Vulkan/DeviceVK.h"

double ProfilerVK::m_TimestampToMillisec = 0.0;
const uint32_t ProfilerVK::m_DashesPerRecurse = 2;
uint32_t ProfilerVK::m_MaxTextWidth = 0;

ProfilerVK::ProfilerVK(const std::string& name, DeviceVK* pDevice)
    :m_Name(name),
    m_pDevice(pDevice),
    m_pProfiledCommandBuffer(nullptr),
    m_RecurseDepth(0),
    m_NextQuery(0),
    m_CurrentFrame(0)
{
    findWidestText();

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        m_ppQueryPools[i] = DBG_NEW QueryPoolVK(pDevice);

        if (!m_ppQueryPools[i]->init(VK_QUERY_TYPE_TIMESTAMP, 100)) {
            LOG("Profiler %s: failed to initialize query pool");
            return;
        }
    }

    // From Vulkan spec: "The number of nanoseconds it takes for a timestamp value to be incremented by 1"
    float nanoSecPerTime = pDevice->getTimestampPeriod();
    const float nanoSecond = std::pow(10.0f, -9.0f);
    const float milliSecondInv = 1000.0f;

    m_TimestampToMillisec = nanoSecPerTime * nanoSecond * milliSecondInv;
}

ProfilerVK::~ProfilerVK()
{
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        SAFEDELETE(m_ppQueryPools[i]);
    }
}

void ProfilerVK::setParentProfiler(ProfilerVK* pParentProfiler)
{
    m_RecurseDepth = pParentProfiler->getRecurseDepth() + 1;
    findWidestText();
}

void ProfilerVK::beginFrame(size_t currentFrame, CommandBufferVK* pProfiledCmdBuffer, CommandBufferVK* pResetCmdBuffer)
{
    if (!m_ProfileFrame) {
        return;
    }

    m_CurrentFrame = currentFrame;
    m_pProfiledCommandBuffer = pProfiledCmdBuffer;

    QueryPoolVK* pCurrentQueryPool = m_ppQueryPools[currentFrame];

    vkCmdResetQueryPool(pResetCmdBuffer->getCommandBuffer(), pCurrentQueryPool->getQueryPool(), 0, pCurrentQueryPool->getQueryCount());
    m_NextQuery = 0;

    // Write a timestamp to measure the time elapsed for the entire scope of the profiler
    vkCmdWriteTimestamp(m_pProfiledCommandBuffer->getCommandBuffer(), VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, pCurrentQueryPool->getQueryPool(), m_NextQuery++);

    // Reset per-frame data
    m_TimeResults.clear();

    for (Timestamp* pTimestamp : m_Timestamps) {
        pTimestamp->queries.clear();
        pTimestamp->time = 0.0f;
    }
}

void ProfilerVK::endFrame()
{
    if (!m_ProfileFrame) {
        return;
    }

    VkQueryPool currentQueryPool = m_ppQueryPools[m_CurrentFrame]->getQueryPool();
    vkCmdWriteTimestamp(m_pProfiledCommandBuffer->getCommandBuffer(), VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, currentQueryPool, m_NextQuery++);

    if (m_NextQuery > m_TimeResults.size()) {
        m_TimeResults.resize(m_NextQuery);
    }
}

void ProfilerVK::writeResults()
{
    if (!m_ProfileFrame) {
        return;
    }

    VkQueryPool currentQueryPool = m_ppQueryPools[m_CurrentFrame]->getQueryPool();

    if (vkGetQueryPoolResults(
        m_pDevice->getDevice(), currentQueryPool,
        0, m_NextQuery,                             // First query, query count
        m_NextQuery * sizeof(uint64_t),             // Data size
        (void*)m_TimeResults.data(),                // Data pointer
        sizeof(uint64_t),                           // Strides
        VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT)
        != VK_SUCCESS)
    {
        LOG("Profiler %s: failed to get query pool results", m_Name.c_str());
        return;
    }

    // Write the profiler's time first
    m_Time = m_TimeResults.back() - m_TimeResults.front();

    for (Timestamp* pTimestamp : m_Timestamps) {
        const std::vector<uint32_t>& timestampQueries = pTimestamp->queries;

        for (size_t queryIdx = 0; queryIdx < timestampQueries.size(); queryIdx += 2) {
            pTimestamp->time += m_TimeResults[timestampQueries[queryIdx + 1]] - m_TimeResults[timestampQueries[queryIdx]];
        }
    }

    // Have the child profilers write their results
    for (ProfilerVK* pChild : m_Children) {
        pChild->writeResults();
    }
}

void ProfilerVK::drawResults()
{
    std::string indent('-', m_RecurseDepth);

    // Align the number across all timestamps and profilers by filling with whitespaces
    uint32_t fillLength = m_MaxTextWidth - (m_RecurseDepth * m_DashesPerRecurse + (uint32_t)m_Name.size());
    std::string whitespaceFill(fillLength, ' ');

    // Convert time to milliseconds
    double timeMs = m_Time * m_TimestampToMillisec;
    ImGui::Text("%s%s:\t%s%f ms", indent.c_str(), m_Name.c_str(), whitespaceFill.c_str(), timeMs);

    // Print timestamps
    uint32_t timestampPrefixWidth = (m_RecurseDepth + 1) * m_DashesPerRecurse;

    for (Timestamp* pTimestamp : m_Timestamps) {
        fillLength = m_MaxTextWidth - (timestampPrefixWidth + (uint32_t)pTimestamp->name.size());
        whitespaceFill = std::string(fillLength, ' ');

        timeMs = pTimestamp->time * m_TimestampToMillisec;
        ImGui::Text("--%s%s:\t%s%f ms", indent.c_str(), pTimestamp->name.c_str(), whitespaceFill.c_str(), timeMs);
    }

    // Draw the child profilers' results
    for (ProfilerVK* pChild : m_Children) {
        pChild->drawResults();
    }
}

void ProfilerVK::addChildProfiler(ProfilerVK* pChildProfiler)
{
    m_Children.push_back(pChildProfiler);
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
    if (!m_ProfileFrame) {
        return;
    }

    pTimestamp->queries.push_back(m_NextQuery);
    vkCmdWriteTimestamp(m_pProfiledCommandBuffer->getCommandBuffer(), VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, m_ppQueryPools[m_CurrentFrame]->getQueryPool(), m_NextQuery++);
}

void ProfilerVK::findWidestText()
{
    // Widest string among the profiler and its timestamps, includes the width of the dash-prefix and the name of the profiler/timestamp
    uint32_t maxTextWidthLocal;
    // Width of profiler's text
    maxTextWidthLocal = m_RecurseDepth * m_DashesPerRecurse + m_Name.size();

    uint32_t timestampPrefixWidth = (m_RecurseDepth + 1) * m_DashesPerRecurse;

    for (Timestamp* pTimestamp : m_Timestamps) {
        maxTextWidthLocal = std::max(timestampPrefixWidth + (uint32_t)pTimestamp->name.size(), maxTextWidthLocal);
    }

    m_MaxTextWidth = std::max(m_MaxTextWidth, maxTextWidthLocal);
}
