#include "IContext.h"
#include "Vulkan/ContextVK.h"

IContext* IContext::create(API api)
{
	static ContextVK vulkanContext;
	
	switch (api)
	{
		case API::VULKAN:
		{
			vulkanContext.init();
			return &vulkanContext;
		}
	}

	return nullptr;
}
