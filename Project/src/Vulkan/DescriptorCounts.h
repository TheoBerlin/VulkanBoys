#pragma once

class DescriptorCounts
{
public:
    unsigned int m_UniformBuffers, m_StorageBuffers, m_SampledImages;

    void enlarge(unsigned int factor)
    {
        m_UniformBuffers *= factor;
        m_StorageBuffers *= factor;
        m_SampledImages *= factor;
    }

    void operator+=(const DescriptorCounts& other)
    {
        m_UniformBuffers += other.m_UniformBuffers;
        m_StorageBuffers += other.m_StorageBuffers;
        m_SampledImages += other.m_SampledImages;
    }

    void operator-=(const DescriptorCounts& other)
    {
        m_UniformBuffers -= other.m_UniformBuffers;
        m_StorageBuffers -= other.m_StorageBuffers;
        m_SampledImages -= other.m_SampledImages;
    }
};
