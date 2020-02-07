#include "ContextVK.h"
#include "ShaderVK.h"

ContextVK::ContextVK()
{
}

ContextVK::~ContextVK()
{
}

void ContextVK::init()
{
	//Instance Init
#if VALIDATION_LAYERS_ENABLED
	m_Instance.addRequiredExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
	
	m_Instance.addRequiredExtension(VK_KHR_SURFACE_EXTENSION_NAME);
#ifdef _WIN32
	m_Instance.addRequiredExtension("VK_KHR_win32_surface");
#endif

	m_Instance.addValidationLayer("VK_LAYER_KHRONOS_validation");
	m_Instance.finalize(VALIDATION_LAYERS_ENABLED);

	//Device Init
	m_Device.addRequiredExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	m_Device.finalize(&m_Instance);
}

IRenderer* ContextVK::createRenderer()
{
	//Todo: Implement
	return nullptr;
}

IShader* ContextVK::createShader()
{
	return new ShaderVK(&m_Device);
}

IBuffer* ContextVK::createBuffer()
{
	//Todo: Implement
	return nullptr;
}

IFrameBuffer* ContextVK::createFrameBuffer()
{
	//Todo: Implement
	return nullptr;
}

IImage* ContextVK::createImage()
{
	//Todo: Implement
	return nullptr;
}

IImageView* ContextVK::createImageView()
{
	//Todo: Implement
	return nullptr;
}

ITexture2D* ContextVK::createTexture2D()
{
	//Todo: Implement
	return nullptr;
}
