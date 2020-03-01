#pragma once

class IProfiler
{
public:
    static void progressTimer(float dt);

protected:
    static bool m_ProfileFrame;

private:
    static const float m_MeasuresPerSecond;

    // Counts up towards the next time profiling will be performed
    static float m_TimeSinceMeasure;
};
