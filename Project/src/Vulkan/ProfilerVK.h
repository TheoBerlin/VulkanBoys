#pragma once

#define NOMINMAX

#include "Common/IProfiler.h"
#include "Vulkan/QueryPoolVK.h"

#include <string>
#include <vector>

class CommandBufferVK;
class DeviceVK;

struct Timestamp {
    std::string name;
    uint64_t time;
    std::vector<uint32_t> queries;
};

class ProfilerVK : public IProfiler
{
public:
    ProfilerVK(const std::string& name, DeviceVK* pDevice, ProfilerVK* pParentProfiler = nullptr);
    ~ProfilerVK();

    void init(CommandBufferVK* m_ppCommandBuffers[]);

    void beginFrame(size_t currentFrame);
    void endFrame();
    void writeResults();
    void drawResults();

    ProfilerVK* createChildProfiler(const std::string& name);

    void initTimestamp(Timestamp* pTimestamp, const std::string name);
    void writeTimestamp(Timestamp* pTimestamp);

    uint32_t getRecurseDepth() const { return m_RecurseDepth; }

private:
    void findWidestText();

private:
    // Multiplying factor used to convert a timestamp unit to milliseconds
    static double m_TimestampToMillisec;
    static const uint32_t m_DashesPerRecurse;
    // The widest string written when rendering profiler results, eg: "--Mesh" is 4 characters wide
    static uint32_t m_MaxTextWidth;

private:
    ProfilerVK* m_pParent;
    // The amount of levels to root level in the profiler tree
    uint32_t m_RecurseDepth;

    std::vector<ProfilerVK*> m_ChildProfilers;
    std::vector<Timestamp*> m_Timestamps;

    std::string m_Name;
    uint64_t m_Time;

    QueryPoolVK* m_ppQueryPools[MAX_FRAMES_IN_FLIGHT];
    DeviceVK* m_pDevice;
    CommandBufferVK** m_ppProfiledCommandBuffers;

    uint32_t m_CurrentFrame, m_NextQuery;
    std::vector<uint64_t> m_TimeResults;
};