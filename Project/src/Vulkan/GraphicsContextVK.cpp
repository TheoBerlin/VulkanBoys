#include "GraphicsContextVK.h"
#include "BufferVK.h"
#include "CopyHandlerVK.h"
#include "ImguiVK.h"
#include "SceneVK.h"
#include "MeshVK.h"
#include "ShaderVK.h"
#include "SamplerVK.h"
#include "SwapChainVK.h"
#include "Texture2DVK.h"
#include "MeshRendererVK.h"
#include "RenderingHandlerVK.h"

#include "Particles/ParticleRendererVK.h"
#include "Particles/ParticleEmitterHandlerVK.h"

#include "Ray Tracing/RayTracingRendererVK.h"

#include "Core/GLFWWindow.h"

GraphicsContextVK::GraphicsContextVK(IWindow* pWindow)
	: m_pWindow(pWindow),
	m_pSwapChain(nullptr),
	m_Device(),
	m_Instance()
{}

GraphicsContextVK::~GraphicsContextVK()
{
	SAFEDELETE(m_pSwapChain);
	m_pWindow = nullptr;

	m_Device.release();
	m_Instance.release();
}

void GraphicsContextVK::init()
{
	//Instance Init
#if VALIDATION_LAYERS_ENABLED
	m_Instance.addRequiredExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

	uint32_t count = 0;
	const char** ppExtensions = glfwGetRequiredInstanceExtensions(&count);
	for (uint32_t i = 0; i < count; i++)
	{
		m_Instance.addRequiredExtension(ppExtensions[i]);
	}

	m_Instance.addOptionalExtension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

	//m_Instance.debugPrintAvailableExtensions();
	//m_Instance.debugPrintAvailableLayers();

	//m_Instance.addValidationLayer("VK_LAYER_RENDERDOC_Capture");
	m_Instance.addValidationLayer("VK_LAYER_KHRONOS_validation");
	m_Instance.finalize(VALIDATION_LAYERS_ENABLED);

	//Device Init
	m_Device.addRequiredExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

	m_Device.addOptionalExtension(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);
	//m_Device.addOptionalExtension(VK_KHR_MAINTENANCE3_EXTENSION_NAME);
	m_Device.addOptionalExtension(VK_NV_RAY_TRACING_EXTENSION_NAME);

	m_Device.finalize(&m_Instance);

	//SwapChain init
	m_pSwapChain = DBG_NEW SwapChainVK(&m_Instance, &m_Device);
	m_pSwapChain->init(m_pWindow, VK_FORMAT_B8G8R8A8_UNORM, MAX_FRAMES_IN_FLIGHT, true);
}

RenderingHandler* GraphicsContextVK::createRenderingHandler()
{
	return DBG_NEW RenderingHandlerVK(this);
}

IRenderer* GraphicsContextVK::createMeshRenderer(RenderingHandler* pRenderingHandler)
{
	return DBG_NEW MeshRendererVK(this, reinterpret_cast<RenderingHandlerVK*>(pRenderingHandler));
}

IRenderer* GraphicsContextVK::createParticleRenderer(RenderingHandler* pRenderingHandler)
{
	return DBG_NEW ParticleRendererVK(this, reinterpret_cast<RenderingHandlerVK*>(pRenderingHandler));
}

IRenderer* GraphicsContextVK::createRayTracingRenderer(RenderingHandler* pRenderingHandler)
{
	return DBG_NEW RayTracingRendererVK(this, reinterpret_cast<RenderingHandlerVK*>(pRenderingHandler));
}

ParticleEmitterHandler* GraphicsContextVK::createParticleEmitterHandler()
{
	return DBG_NEW ParticleEmitterHandlerVK();
}

IImgui* GraphicsContextVK::createImgui()
{
	return DBG_NEW ImguiVK(this);
}

IScene* GraphicsContextVK::createScene()
{
	return DBG_NEW SceneVK(this);
}

IMesh* GraphicsContextVK::createMesh()
{
	return DBG_NEW MeshVK(&m_Device);
}

IShader* GraphicsContextVK::createShader()
{
	return DBG_NEW ShaderVK(&m_Device);
}

IBuffer* GraphicsContextVK::createBuffer()
{
	return DBG_NEW BufferVK(&m_Device);
}

void GraphicsContextVK::updateBuffer(IBuffer* pDestination, uint64_t destinationOffset, const void* pSource, uint64_t sizeInBytes)
{
	BufferVK* pDestBuffer = reinterpret_cast<BufferVK*>(pDestination);
	m_Device.getCopyHandler()->updateBuffer(pDestBuffer, destinationOffset, pSource, sizeInBytes);
}

IFrameBuffer* GraphicsContextVK::createFrameBuffer()
{
	//Todo: Implement
	return nullptr;
}

IImage* GraphicsContextVK::createImage()
{
	//Todo: Implement
	return nullptr;
}

IImageView* GraphicsContextVK::createImageView()
{
	//Todo: Implement
	return nullptr;
}

ITexture2D* GraphicsContextVK::createTexture2D()
{
	return DBG_NEW Texture2DVK(&m_Device);
}

ISampler* GraphicsContextVK::createSampler()
{
	return new SamplerVK(&m_Device);
}

void GraphicsContextVK::sync()
{
	m_Device.wait();
}

void GraphicsContextVK::swapBuffers(VkSemaphore renderSemaphore)
{
	m_pSwapChain->present(renderSemaphore);
}

bool GraphicsContextVK::supportsRayTracing() const
{
	return m_Device.supportsRayTracing();
}
