#include "Texture2DGL.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

Texture2DGL::Texture2DGL() {}

Texture2DGL::~Texture2DGL()
{
	if (textureHandle != 0)
	{
		glDeleteTextures(1, &textureHandle);
		fprintf(stderr,"texture deleted\n");
	};
}

// return 0 if image was loaded and texture created.
// else return -1
int Texture2DGL::loadFromFile(std::string filename)
{
	int w, h, bpp;
	unsigned char* rgb = stbi_load(filename.c_str(), &w, &h, &bpp, STBI_rgb_alpha);
	if (rgb == nullptr)
	{
		fprintf(stderr, "Error loading texture file: %s\n", filename.c_str());
		return -1;
	}

	// not 0
	if (textureHandle)
	{
		glDeleteTextures(1, &textureHandle);
	};

	glGenTextures(1, &textureHandle);
	glBindTexture(GL_TEXTURE_2D, textureHandle);

	// OpenGL default texture sampler, used when no texture sampler
	// is specified.
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

//	if (bpp == 3)
//		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, rgb);
//	else if (bpp == 4)
	// for now only RGBA
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgb);

	glGenerateMipmap(GL_TEXTURE_2D);

	// unbind texture
	glBindTexture(GL_TEXTURE_2D, 0);

	stbi_image_free(rgb);
	return 0;
}

void Texture2DGL::bind(unsigned int slot)
{
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, textureHandle);

	if (this->sampler != nullptr)
	{
		Sampler2DGL* s = (Sampler2DGL*)this->sampler;
		glBindSampler(slot, s->samplerHandler);
		glSamplerParameteri(s->samplerHandler, GL_TEXTURE_MAG_FILTER, s->magFilter);
		glSamplerParameteri(s->samplerHandler, GL_TEXTURE_MIN_FILTER, s->minFilter);
		glSamplerParameteri(s->samplerHandler, GL_TEXTURE_WRAP_S, s->wrapS);
		glSamplerParameteri(s->samplerHandler, GL_TEXTURE_WRAP_T, s->wrapT);
	}
}
