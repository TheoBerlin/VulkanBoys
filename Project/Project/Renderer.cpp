#include "Vulkan/VulkanRenderer.h"
#include "Renderer.h"


Renderer* Renderer::makeRenderer(BACKEND option)
{
	if (option == BACKEND::VULKAN) {
		return new VulkanRenderer();
	}
}

