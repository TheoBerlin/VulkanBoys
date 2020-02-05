#include "VertexBufferGL.h"
#include "MeshGL.h"
#include <assert.h>

GLuint VertexBufferGL::usageMapping[3] = { GL_STATIC_COPY, GL_DYNAMIC_COPY, GL_DONT_CARE };

/*
	Create a VertexBuffer backed in gpu mem
	- todo: add error checking...
*/
VertexBufferGL::VertexBufferGL(size_t size, DATA_USAGE usage) {
	totalSize = size;
	GLuint newSSBO = 0;
	glGenBuffers(1, &newSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, newSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, size, nullptr, usageMapping[usage]);
	unbind();
	_handle = newSSBO;
}

VertexBufferGL::~VertexBufferGL()
{
	glDeleteBuffers(1, &_handle);
}

/*
	SSBO based Vertex Buffer
*/
void VertexBufferGL::setData(const void* data, size_t size,  size_t offset)
{
	assert(size + offset <= totalSize);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, _handle);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, size, data);
	unbind();
}

/*
 bind at "location", with offset "offset", "size" bytes 
 */
void VertexBufferGL::bind(size_t offset, size_t size, unsigned int location) {
	assert(offset + size <= totalSize);
	glBindBufferRange(GL_SHADER_STORAGE_BUFFER, location, _handle, offset, size);
}

inline void VertexBufferGL::unbind() {
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

inline size_t VertexBufferGL::getSize() {
	return totalSize;
}
