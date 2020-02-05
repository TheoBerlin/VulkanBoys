#include <GL/glew.h>

#include "Sampler2DGL.h"

// enum WRAPPING { REPEAT = 0, CLAMP = 1 };
// enum FILTER { POINT = 0, LINEAR = 0 };
GLuint wrapMap[2] = { GL_REPEAT, GL_CLAMP };
GLuint filterMap[2] = { GL_NEAREST, GL_LINEAR };


Sampler2DGL::Sampler2DGL()
{
	glGenSamplers(1, &samplerHandler);
	// defaults
	minFilter = magFilter = GL_NEAREST;
	wrapS = wrapT = GL_CLAMP;
}

Sampler2DGL::~Sampler2DGL()
{
	glDeleteSamplers(1, &samplerHandler);
}

void Sampler2DGL::setMagFilter(FILTER filter)
{
	magFilter = filterMap[filter];
}


void Sampler2DGL::setMinFilter(FILTER filter) 
{
	minFilter = filterMap[filter];
}

void Sampler2DGL::setWrap(WRAPPING s, WRAPPING t)
{
	wrapS = wrapMap[s];
	wrapT = wrapMap[t];
}

