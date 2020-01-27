#pragma once
#include <GL/glew.h>
#include "../ConstantBuffer.h"

class ConstantBufferGL : public ConstantBuffer
{
public:
	ConstantBufferGL(std::string NAME, unsigned int location);
	~ConstantBufferGL();
	void setData(const void* data, size_t size, Material* m, unsigned int location);
	void bind(Material*);

private:

	std::string name;
	GLuint location;
	GLuint handle;
	GLuint index;
	void* buff = nullptr;
	void* lastMat;
};

