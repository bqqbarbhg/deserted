#include "sandbox.h"
#include <stdint.h>
#include <vector>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <GL/glew.h>
#include <GL/GL.h>

#include <assert.h>

#include "../ext/stb_image.h"

#if defined(_WIN32)
#include <Windows.h>
#endif

namespace sb
{

struct PassImpl : Pass
{
	std::string vertFile;
	std::string fragFile;
	pp::Pass *pass;
	uint64_t timestamp;
};

struct SandboxImpl : public Sandbox
{
	std::unordered_set<Pass*> passes;

	void loadPassImpl(PassImpl *p);
};

#if defined(_WIN32)
uint64_t getFileTimestamp(const char *filename)
{
	HANDLE file = CreateFileA(filename,
			GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (file != INVALID_HANDLE_VALUE)
	{
		FILETIME time = { 0, 0 };
		GetFileTime(file, NULL, NULL, &time);
		CloseHandle(file);

		return (uint64_t)time.dwHighDateTime << 32
			| (uint64_t)time.dwLowDateTime;
	}
	else
	{
		return 0;
	}
}
#endif

std::string readFile(const char *filename)
{
	FILE *f = fopen(filename, "rb");
	if (!f) return std::string();

	fseek(f, 0, SEEK_END);
	size_t size = ftell(f);
	fseek(f, 0, SEEK_SET);

	std::string s;
	s.resize(size);
	fread(&s[0], 1, size, f);
	fclose(f);

	return s;
}

void SandboxImpl::loadPassImpl(PassImpl *pi)
{
	uint64_t ts;
	ts = getFileTimestamp(pi->vertFile.c_str());
	if (ts > pi->timestamp) pi->timestamp = ts;
	ts = getFileTimestamp(pi->fragFile.c_str());
	if (ts > pi->timestamp) pi->timestamp = ts;

	std::string vert = readFile(pi->vertFile.c_str());
	std::string frag = readFile(pi->fragFile.c_str());

	if (pi->pass)
		ctx->destroyPass(pi->pass);
	pi->pass = ctx->createPass(vert.c_str(), frag.c_str());
}

Pass *Sandbox::loadPass(const char *vertFile, const char *fragFile)
{
	SandboxImpl *impl = (SandboxImpl*)this;
	PassImpl *pi = new PassImpl();

	pi->vertFile = std::string(vertFile);
	pi->fragFile = std::string(fragFile);

	pi->pass = 0;
	impl->loadPassImpl(pi);

	impl->passes.insert(pi);
	
	return pi;
}

void Sandbox::destroyPass(Pass *p)
{
	SandboxImpl *impl = (SandboxImpl*)this;
	PassImpl *pi = (PassImpl*)p;

	auto it = impl->passes.find(p);
	assert(it != impl->passes.end());
	impl->passes.erase(it);

	ctx->destroyPass(pi->pass);

	delete pi;
}

pp::Texture *Sandbox::loadImage(const char *file)
{
	int w, h;

	stbi_uc *data = stbi_load(file, &w, &h, 0, 4);
	pp::Texture *tex = ctx->createImage(data, w, h);
	stbi_image_free(data);

	return tex;
}

void Sandbox::hotloadShaders()
{
	SandboxImpl *impl = (SandboxImpl*)this;

	for (auto p : impl->passes)
	{
		PassImpl *pi = (PassImpl*)p;

		uint64_t maxTs = 0;
		uint64_t ts;
		ts = getFileTimestamp(pi->vertFile.c_str());
		if (ts > maxTs) maxTs = ts;
		ts = getFileTimestamp(pi->fragFile.c_str());
		if (ts > maxTs) maxTs = ts;

		if (pi->timestamp < maxTs)
		{
			impl->loadPassImpl(pi);
		}
	}
}

void Sandbox::renderPass(Pass *p, pp::Texture *target)
{
	PassImpl *pi = (PassImpl*)p;
	ctx->renderQuad(pi->pass, target);
}

Sandbox *createSandbox(pp::Context *ctx)
{
	SandboxImpl *si = new SandboxImpl();

	si->ctx = ctx;

	return si;
}

void destroySandbox(Sandbox *s)
{
	SandboxImpl *si = (SandboxImpl*)s;

	delete si;
}

}

