#pragma once
#include <GL/glew.h>
#include "../VertexBuffer.h"

class VertexBufferGL :
	public VertexBuffer
{
public:
	
	VertexBufferGL(size_t size, VertexBuffer::DATA_USAGE usage);
	~VertexBufferGL();
	
	void setData(const void* data, size_t size, size_t offset);
	void bind(size_t offset, size_t size, unsigned int location);
	void unbind();
	size_t getSize();

	static GLuint usageMapping[3];

private:
	size_t totalSize;
	GLuint _handle;
};

