#pragma once
#include "../Sampler2D.h"

class Sampler2DGL :
	public Sampler2D
{
public:
	Sampler2DGL();
	~Sampler2DGL();
	void setMagFilter(FILTER filter);
	void setMinFilter(FILTER filter);
	void setWrap(WRAPPING s, WRAPPING t);

	GLuint magFilter, minFilter, wrapS, wrapT;
	GLuint samplerHandler = 0;
private:
};

