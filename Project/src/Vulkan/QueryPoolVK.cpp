#include "QueryPoolVK.h"
#include "DeviceVK.h"

QueryPoolVK::QueryPoolVK(DeviceVK* pDevice) :
	m_pDevice(pDevice),
	m_QueryPool(VK_NULL_HANDLE),
	m_QueryCount(0)
{
}

QueryPoolVK::~QueryPoolVK()
{
	if (m_QueryPool != VK_NULL_HANDLE)
	{
		vkDestroyQueryPool(m_pDevice->getDevice(), m_QueryPool, nullptr);
		m_QueryPool = VK_NULL_HANDLE;
	}
}

bool QueryPoolVK::init(VkQueryType queryType, uint32_t queryCount, VkQueryPipelineStatisticFlags statisticsFlags)
{
	VkQueryPoolCreateInfo createInfo = {};
	createInfo.sType				= VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
	createInfo.pNext				= nullptr;
	createInfo.queryType			= queryType;
	createInfo.queryCount			= queryCount;
	createInfo.pipelineStatistics	= statisticsFlags;

	VK_CHECK_RESULT_RETURN_FALSE(vkCreateQueryPool(m_pDevice->getDevice(), &createInfo, nullptr, &m_QueryPool), "--- QueryPoolVK: vkCreateQueryPool failed!");

	m_QueryCount = queryCount;
	D_LOG("--- QueryPoolVK: QueryPool created successfully!");
	return true;
}
