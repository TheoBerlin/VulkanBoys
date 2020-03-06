#include "Profiler.h"

#include <cmath>

bool Profiler::m_ProfileFrame = true;
const float Profiler::m_MeasuresPerSecond = 2.0f;
float Profiler::m_TimeSinceMeasure = 1.0f / m_MeasuresPerSecond;

void Profiler::progressTimer(float dt)
{
    if (m_TimeSinceMeasure > 1.0f / m_MeasuresPerSecond) {
        // The last frame was profiled, skip this one
        m_ProfileFrame = false;

        m_TimeSinceMeasure = std::fmod(m_TimeSinceMeasure, 1.0f / m_MeasuresPerSecond);
    }

    m_TimeSinceMeasure += dt;

    if (m_TimeSinceMeasure > 1.0f / m_MeasuresPerSecond) {
        m_ProfileFrame = true;
    }
}
