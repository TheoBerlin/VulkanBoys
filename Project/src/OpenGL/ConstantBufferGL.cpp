#include "ConstantBufferGL.h"
#include "MaterialGL.h"

ConstantBufferGL::ConstantBufferGL(std::string NAME, unsigned int location) 
{
	name = NAME;
	handle = 0;
	// make a CPU side buffer.
	// binding point (between BUFFER AND SHADER PROGRAM)
	this->location = location;
	// location of buffer definition in SHADER PROGRAM
	index = GL_MAX_UNIFORM_LOCATIONS;
	lastMat = nullptr;
}

ConstantBufferGL::~ConstantBufferGL()
{
	if (handle != 0)
	{
		glDeleteBuffers(1, &handle);
		handle = 0;
	};
}

// this allows us to not know in advance the type of the receiving end, vec3, vec4, etc.
void ConstantBufferGL::setData(const void* data, size_t size, Material* m, unsigned int location)
{
	if (handle == 0)
	{
		glGenBuffers(1, &handle);
		glBindBuffer(GL_UNIFORM_BUFFER, handle);
		glBufferData(GL_UNIFORM_BUFFER, size, data, GL_STATIC_DRAW);
	}
	else
		glBindBuffer(GL_UNIFORM_BUFFER, handle);

	void* dest = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
	memcpy(dest, data, size);
	glUnmapBuffer(GL_UNIFORM_BUFFER);

	//if (buff == nullptr)
	//{
	//	buff = new char[size];
	//}

	// index of uniform block has not been found yet, or we are using this buffer
	// with another material!
	// otherwise we already know the index!, it's fixed at compile time
	if (index == GL_MAX_UNIFORM_LOCATIONS || lastMat != m)
	{
		lastMat = m;
		MaterialGL* mat = (MaterialGL*)m;
		index = glGetUniformBlockIndex(mat->getProgram(), name.c_str());
		//index = glGetUniformBlockIndex(mat->getProgram(), "TranslationBlock");

	}
	// bind CPU side
	//glBindBuffer(GL_UNIFORM_BUFFER,handle);
	// set data
	// set BINDING POINT
	// stop affecting this buffer.
	glBindBuffer(GL_UNIFORM_BUFFER,0);
}

void ConstantBufferGL::bind(Material* m)
{
	glBindBuffer(GL_UNIFORM_BUFFER, handle);
	glBindBufferBase(GL_UNIFORM_BUFFER, location, handle);
}

/*
	//if (_index = GL_MAX_UNIFORM_LOCATIONS)
	//{
	//	MaterialGL* material = (MaterialGL*)m;
	//	_index = glGetUniformLocation(material->getProgram(), "translate");
	//}
	//if (_index == 0)
	// _index = glGetUniformBlockIndex(material->getProgram(), "transform");
	//glUniformBlockBinding(material->getProgram(), _index, _location);
	//glUniform4fv(_location, 1, (float*)_buff);
}
*/

