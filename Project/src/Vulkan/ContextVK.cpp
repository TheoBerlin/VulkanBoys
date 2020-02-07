#include "ContextVK.h"
#include "ShaderVK.h"
#include "RendererVK.h"
#include "SwapChainVK.h"

#include "Core/GLFWWindow.h"

ContextVK::ContextVK(IWindow* pWindow)
	: m_pWindow(pWindow),
	m_pSwapChain(nullptr),
	m_Device(),
	m_Instance()
{
}

ContextVK::~ContextVK()
{
	SAFEDELETE(m_pSwapChain);
	m_pWindow = nullptr;
}

void ContextVK::init()
{
	//Instance Init
#if VALIDATION_LAYERS_ENABLED
	m_Instance.addRequiredExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
	
	GLFWwindow* pNativeWindow = reinterpret_cast<GLFWwindow*>(m_pWindow->getNativeHandle());

	uint32_t count = 0;
	const char** ppExtensions = glfwGetRequiredInstanceExtensions(&count);
	for (uint32_t i = 0; i < count; i++)
	{
		m_Instance.addRequiredExtension(ppExtensions[i]);
	}

	m_Instance.addValidationLayer("VK_LAYER_KHRONOS_validation");
	m_Instance.finalize(VALIDATION_LAYERS_ENABLED);

	//Device Init
	m_Device.addRequiredExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	m_Device.finalize(&m_Instance);

	//SwapChain init
	m_pSwapChain = new SwapChainVK(&m_Instance, &m_Device);
	m_pSwapChain->init(m_pWindow, VK_FORMAT_B8G8R8A8_UNORM, MAX_FRAMES_IN_FLIGHT, true);
}

IRenderer* ContextVK::createRenderer()
{
	return new RendererVK(this);
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

void ContextVK::swapBuffers(VkSemaphore renderSemaphore)
{
	m_pSwapChain->present(renderSemaphore);
}
