#include "OpenGL/OpenGLRenderer.h"
#include "Renderer.h"


Renderer* Renderer::makeRenderer(BACKEND option)
{
	if (option == BACKEND::GL45)
		return new OpenGLRenderer();
}

