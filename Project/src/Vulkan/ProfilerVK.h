#pragma once

#include "QueryPoolVK.h"

#include <string>
#include <vector>

class CommandBufferVK;
class DeviceVK;

struct Timestamp {
    std::string name;
    uint64_t time;
    std::vector<uint32_t> queries;
};

const float g_MeasuresPerSecond = 2.0f;

class ProfilerVK
{
public:
    ProfilerVK(const std::string& name, DeviceVK* pDevice, ProfilerVK* pParentProfiler = nullptr);
    ~ProfilerVK();

    void init(CommandBufferVK* m_ppCommandBuffers[]);

    void beginFrame(size_t currentFrame, float dt);
    // Called to the root profiler
    void endFrame();
    void writeResults();
    void drawResults(uint32_t indentLength = 0);

    ProfilerVK* createChildProfiler(const std::string& name);

    void initTimestamp(Timestamp* pTimestamp, const std::string name);
    void writeTimestamp(Timestamp* pTimestamp);

private:
    ProfilerVK* m_pParent;
    std::vector<ProfilerVK*> m_ChildProfilers;
    std::vector<Timestamp*> m_Timestamps;

    std::string m_Name;
    uint64_t m_Time;

    QueryPoolVK* m_ppQueryPools[MAX_FRAMES_IN_FLIGHT];
    DeviceVK* m_pDevice;
    CommandBufferVK** m_ppProfiledCommandBuffers;

    uint32_t m_CurrentFrame, m_NextQuery;
    std::vector<uint64_t> m_TimeResults;

    // Multiplying factor used to convert a timestamp unit to milliseconds
    static double m_TimestampToMillisec;

    float m_TimeSinceMeasure;
};
