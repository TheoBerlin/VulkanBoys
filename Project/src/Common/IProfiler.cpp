#include "IProfiler.h"

#include <cmath>

bool IProfiler::m_ProfileFrame = true;
const float IProfiler::m_MeasuresPerSecond = 2.0f;
float IProfiler::m_TimeSinceMeasure = 1.0f / m_MeasuresPerSecond;

void IProfiler::progressTimer(float dt)
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
