#pragma once

class Profiler
{
public:
    static void progressTimer(float dt);
    virtual double getElapsedTime() const = 0;

protected:
    static bool m_ProfileFrame;

private:
    static const float m_MeasuresPerSecond;

    // Counts up towards the next time profiling will be performed
    static float m_TimeSinceMeasure;
};
