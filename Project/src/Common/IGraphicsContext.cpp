#include "IGraphicsContext.h"
#include "Vulkan/GraphicsContextVK.h"

IGraphicsContext* IGraphicsContext::create(IWindow* pWindow, API api)
{
	switch (api)
	{
		case API::VULKAN:
		{
			GraphicsContextVK* pContext = DBG_NEW GraphicsContextVK(pWindow);
			pContext->init();
			return pContext;
		}
	}

	return nullptr;
}
