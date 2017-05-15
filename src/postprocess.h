#pragma once

#include <stdint.h>

namespace pp
{

struct Texture
{
	uint32_t getWidth() const;
	uint32_t getHeight() const;
};

struct Pass
{
};

struct Context
{
	Pass *createPass(const char *vert, const char *frag);
	void destroyPass(Pass *p);

	Texture *createRenderTarget(uint32_t width, uint32_t height);
	Texture *createImage(const void *data, uint32_t width, uint32_t height);
	void destroyTexture(Texture *t);

	void setVector(const char *name, const float *values, uint32_t num);
	void setTexture(const char *name, Texture *t);

	void readPixels(Texture *t, void *pixels);

	void setBackbufferSize(uint32_t width, uint32_t height);

	void clear(Texture *target, float r, float g, float b, float a);
	void renderQuad(Pass *p, Texture *target);
};

Context *createContext();
void destroyContext(Context *c);

};

