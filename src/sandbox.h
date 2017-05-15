#pragma once

#include "postprocess.h"

namespace sb
{

struct Pass
{
};

struct Sandbox
{
	pp::Context *ctx;

	Pass *loadPass(const char *vertFile, const char *fragFile);
	void destroyPass(Pass *p);

	pp::Texture *loadImage(const char *file);

	void hotloadShaders();

	void renderPass(Pass *p, pp::Texture *target);
};

Sandbox *createSandbox(pp::Context *ctx);
void destroySandbox(Sandbox *s);

}

