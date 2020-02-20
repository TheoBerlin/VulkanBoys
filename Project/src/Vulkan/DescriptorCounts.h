#pragma once

class DescriptorCounts
{
public:
    unsigned int m_UniformBuffers, m_StorageBuffers, m_SampledImages, m_StorageImages, m_AccelerationStructures;

    void enlarge(unsigned int factor)
    {
        m_UniformBuffers *= factor;
        m_StorageBuffers *= factor;
        m_SampledImages *= factor;
		m_StorageImages *= factor;
    	m_AccelerationStructures *= factor;
    }

    size_t getDescriptorTypesCount() const
    {
        return (m_UniformBuffers > 0) + (m_StorageBuffers > 0) + (m_SampledImages > 0) + (m_StorageImages > 0)
            + (m_AccelerationStructures > 0);
    }

    void operator+=(const DescriptorCounts& other)
    {
        m_UniformBuffers += other.m_UniformBuffers;
        m_StorageBuffers += other.m_StorageBuffers;
        m_SampledImages += other.m_SampledImages;
		m_StorageImages += other.m_StorageImages;
		m_AccelerationStructures += other.m_AccelerationStructures;
    }

    void operator-=(const DescriptorCounts& other)
    {
        m_UniformBuffers -= other.m_UniformBuffers;
        m_StorageBuffers -= other.m_StorageBuffers;
        m_SampledImages -= other.m_SampledImages;
		m_StorageImages -= other.m_StorageImages;
		m_AccelerationStructures -= other.m_AccelerationStructures;
    }
};
