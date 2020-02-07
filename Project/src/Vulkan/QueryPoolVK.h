#pragma once
#include "VulkanCommon.h"

class DeviceVK;
class QueryVK;

class QueryPoolVK
{
public:
	DECL_NO_COPY(QueryPoolVK);
	
	QueryPoolVK(DeviceVK* pDevice);
	~QueryPoolVK();

	bool init(VkQueryType queryType, uint32_t queryCount, VkQueryPipelineStatisticFlags statisticsFlags = 0);

	VkQueryPool getQueryPool() { return m_QueryPool; }

private:
	DeviceVK* m_pDevice;

	VkQueryPool m_QueryPool;
};