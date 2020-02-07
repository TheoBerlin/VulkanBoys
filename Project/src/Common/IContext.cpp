#include "IContext.h"
#include "Vulkan/ContextVK.h"

IContext* IContext::create(IWindow* pWindow, API api)
{
	switch (api)
	{
		case API::VULKAN:
		{
			ContextVK* pContext = new ContextVK(pWindow);
			pContext->init();
			return pContext;
		}
	}

	return nullptr;
}
