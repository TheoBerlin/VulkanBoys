#include <GL/glew.h>
#include "RenderStateGL.h"

RenderStateGL::RenderStateGL()
{
	_wireframe = false;
}

RenderStateGL::~RenderStateGL()
{
}

void RenderStateGL::set()
{
	// was wireframe mode already set?
	if (*globalWireFrame == _wireframe)
		return;
	else
		*globalWireFrame = _wireframe;

	if (_wireframe)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // change to wireframe
	else
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);	// change to solid
}

/*
	Keep a pointer to the global wireframe state
*/
void RenderStateGL::setGlobalWireFrame(bool* global)
{
	this->globalWireFrame = global;
}

/*
 set wireframe mode for this Render state
*/
void RenderStateGL::setWireFrame(bool wireframe) {
	_wireframe = wireframe;

}
