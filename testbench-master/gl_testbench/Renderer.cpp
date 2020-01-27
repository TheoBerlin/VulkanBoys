#include "OpenGL/OpenGLRenderer.h"
#include "Vulkan/VulkanRenderer.h"
#include "Renderer.h"


Renderer* Renderer::makeRenderer(BACKEND option)
{
	if (option == BACKEND::GL45) {
		return new OpenGLRenderer();
	} else if (option == BACKEND::VULKAN) {
		return nullptr;// new VulkanRenderer();
	}
}

