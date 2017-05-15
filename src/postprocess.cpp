#include "postprocess.h"
#include <stdint.h>
#include <vector>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <assert.h>
#include <GL/glew.h>
#include <GL/GL.h>

namespace pp
{

struct TextureImpl : Texture
{
	uint32_t width, height;

	GLuint texture;
	GLuint framebuffer;
};

struct Bind
{
	std::string name;
	GLint loc;
};

struct PassImpl : Pass
{
	GLint vert;
	GLint frag;
	GLint program;

	std::vector<Bind> vec[4];
	std::vector<Bind> tex;
	std::vector<Bind> attrib;
};

struct VectorVar
{
	float values[4];
	uint32_t num;
};

struct TextureVar
{
	TextureImpl *tex;
};

struct Attrib
{
	GLint size;
	GLenum type;
	GLboolean normalized;
	GLsizei stride;
	uint32_t offset;
};

struct ContextImpl : Context
{
	std::unordered_set<Pass*> passes;
	std::unordered_set<Texture*> textures;

	std::unordered_map<std::string, VectorVar> vectorVars;
	std::unordered_map<std::string, TextureVar> textureVars;
	std::unordered_map<std::string, Attrib> attribs;

	GLuint quadVerts;
	GLuint quadIndices;

	uint32_t backbufferWidth;
	uint32_t backbufferHeight;

	uint32_t bindWidth;
	uint32_t bindHeight;

	void bindTarget(Texture *target);
	float *findVec(const std::string& name);
};

uint32_t Texture::getWidth() const
{
	TextureImpl *impl = (TextureImpl*)this;
	return impl->width;
}

uint32_t Texture::getHeight() const
{
	TextureImpl *impl = (TextureImpl*)this;
	return impl->height;
}

char infoLog[16 * 1024];

GLint createShader(GLenum type, const char *src)
{
	GLint shader = glCreateShader(type);
	glShaderSource(shader, 1, (const GLchar**)&src, NULL);
	glCompileShader(shader);

	GLint success = GL_FALSE;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (success != GL_TRUE)
	{
		glGetShaderInfoLog(shader, sizeof(infoLog), 0, (GLchar*)infoLog);
		fprintf(stderr, "Shader compile fail:\n%s\n", infoLog);
	}

	return shader;
}

GLint linkProgram(GLint vert, GLint frag)
{
	GLint program = glCreateProgram();
	glAttachShader(program, vert);
	glAttachShader(program, frag);
	glLinkProgram(program);

	GLint success = GL_FALSE;
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (success != GL_TRUE)
	{
		glGetProgramInfoLog(program, sizeof(infoLog), 0, (GLchar*)infoLog);
		fprintf(stderr, "Program link fail:\n%s\n", infoLog);
	}

	return program;
}

Pass *Context::createPass(const char *vert, const char *frag)
{
	ContextImpl *impl = (ContextImpl*)this;
	PassImpl *pi = new PassImpl();

	pi->vert = createShader(GL_VERTEX_SHADER, vert);
	pi->frag = createShader(GL_FRAGMENT_SHADER, frag);
	pi->program = linkProgram(pi->vert, pi->frag);

	int numUniform;
	glGetProgramiv(pi->program, GL_ACTIVE_UNIFORMS, &numUniform);
	for (int i = 0; i < numUniform; i++)
	{
		GLsizei len;
		GLint size;
		GLenum type;
		char name[256];
		glGetActiveUniform(pi->program, i, sizeof(name), &len, &size, &type, name);

		Bind b;
		b.loc = i;
		b.name = name;

		switch (type)
		{
		case GL_FLOAT:
			pi->vec[0].push_back(b);
			break;
		case GL_FLOAT_VEC2:
			pi->vec[1].push_back(b);
			break;
		case GL_FLOAT_VEC3:
			pi->vec[2].push_back(b);
			break;
		case GL_FLOAT_VEC4:
			pi->vec[3].push_back(b);
			break;
		case GL_SAMPLER_2D:
			pi->tex.push_back(b);
			break;

		default:
			assert(0 && "Unknown uniform type");
		}
	}

	int numAttrib;
	glGetProgramiv(pi->program, GL_ACTIVE_ATTRIBUTES, &numAttrib);
	for (int i = 0; i < numAttrib; i++)
	{
		GLsizei len;
		GLint size;
		GLenum type;
		char name[256];
		glGetActiveAttrib(pi->program, i, sizeof(name), &len, &size, &type, name);

		Bind b;
		b.loc = i;
		b.name = name;
		pi->attrib.push_back(b);
	}

	impl->passes.insert(pi);

	return pi;
}

void Context::destroyPass(Pass *p)
{
	ContextImpl *impl = (ContextImpl*)this;
	PassImpl *pi = (PassImpl*)p;

	{
		auto it = impl->passes.find(p);
		assert(it != impl->passes.end());
		impl->passes.erase(it);
	}

	glDeleteShader(pi->vert);
	glDeleteShader(pi->frag);
	glDeleteProgram(pi->program);

	delete pi;
}

Texture *Context::createRenderTarget(uint32_t width, uint32_t height)
{
	ContextImpl *impl = (ContextImpl*)this;
	TextureImpl *ti = new TextureImpl();

	ti->width = width;
	ti->height = height;

	glGenTextures(1, &ti->texture);
	glBindTexture(GL_TEXTURE_2D, ti->texture);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glGenFramebuffers(1, &ti->framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, ti->framebuffer);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ti->texture, 0);

	impl->textures.insert(ti);

	return ti;
}

Texture *Context::createImage(const void *data, uint32_t width, uint32_t height)
{
	ContextImpl *impl = (ContextImpl*)this;
	TextureImpl *ti = new TextureImpl();

	ti->width = width;
	ti->height = height;

	glGenTextures(1, &ti->texture);
	glBindTexture(GL_TEXTURE_2D, ti->texture);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	ti->framebuffer = 0;

	impl->textures.insert(ti);

	return ti;
}

void Context::destroyTexture(Texture *t)
{
	ContextImpl *impl = (ContextImpl*)this;
	TextureImpl *ti = (TextureImpl*)t;

	{
		auto it = impl->textures.find(t);
		assert(it != impl->textures.end());
		impl->textures.erase(it);
	}

	glDeleteTextures(1, &ti->texture);

	if (ti->framebuffer)
		glDeleteFramebuffers(1, &ti->framebuffer);

	delete ti;
}

void Context::setVector(const char *name, const float *values, uint32_t num)
{
	ContextImpl *impl = (ContextImpl*)this;

	VectorVar &vv = impl->vectorVars[std::string(name)];
	if (num > 4) num = 4;
	for (uint32_t i = 0; i < num; i++)
		vv.values[i] = values[i];
	for (uint32_t i = num; i < 4; i++)
		vv.values[i] = 0.0f;
	vv.num = num;
}

void Context::setTexture(const char *name, Texture *t)
{
	ContextImpl *impl = (ContextImpl*)this;

	TextureVar &tv = impl->textureVars[std::string(name)];
	tv.tex = (TextureImpl*)t;
}

void Context::readPixels(Texture *t, void *pixels)
{
	ContextImpl *impl = (ContextImpl*)this;

	impl->bindTarget(t);
	glReadPixels(0, 0, impl->bindWidth, impl->bindHeight, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
}

float nullVec[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

float *ContextImpl::findVec(const std::string& name)
{
	auto it = this->vectorVars.find(name);
	if (it != this->vectorVars.end())
		return it->second.values;
	else
		return nullVec;
}

void Context::setBackbufferSize(uint32_t width, uint32_t height)
{
	ContextImpl *impl = (ContextImpl*)this;
	impl->backbufferWidth = width;
	impl->backbufferHeight = height;
}

void ContextImpl::bindTarget(Texture *target)
{
	TextureImpl *targetImpl = (TextureImpl*)target;
	if (targetImpl)
	{
		assert(targetImpl->framebuffer != 0);
		glBindFramebuffer(GL_FRAMEBUFFER, targetImpl->framebuffer); 

		bindWidth = targetImpl->width;
		bindHeight = targetImpl->height;
	}
	else
	{
		assert(this->backbufferWidth > 0 && this->backbufferHeight > 0);

		glBindFramebuffer(GL_FRAMEBUFFER, 0); 
		bindWidth = this->backbufferWidth;
		bindHeight = this->backbufferHeight;
	}

	glViewport(0, 0, bindWidth, bindHeight);
}

void Context::clear(Texture *target, float r, float g, float b, float a)
{
	ContextImpl *impl = (ContextImpl*)this;
	impl->bindTarget(target);

	glClearColor(r, g, b, a);
	glClear(GL_COLOR_BUFFER_BIT);
}

void Context::renderQuad(Pass *p, Texture *target)
{
	ContextImpl *impl = (ContextImpl*)this;
	PassImpl *pi = (PassImpl*)p;

	glUseProgram(pi->program);

	for (auto& u : pi->vec[0])
		glUniform1fv(u.loc, 1, impl->findVec(u.name));
	for (auto& u : pi->vec[1])
		glUniform2fv(u.loc, 1, impl->findVec(u.name));
	for (auto& u : pi->vec[2])
		glUniform3fv(u.loc, 1, impl->findVec(u.name));
	for (auto& u : pi->vec[3])
		glUniform4fv(u.loc, 1, impl->findVec(u.name));

	GLint texIndex = 0;
	for (auto& u : pi->tex)
	{
		GLint bind = 0;
		auto it = impl->textureVars.find(u.name);
		if (it != impl->textureVars.end())
			bind = it->second.tex->texture;

		glActiveTexture(GL_TEXTURE0 + texIndex);
		glBindTexture(GL_TEXTURE_2D, bind);
		glUniform1i(u.loc, texIndex);
		texIndex++;
	}
	glActiveTexture(GL_TEXTURE0);

	for (auto& a : pi->attrib)
	{
		auto it = impl->attribs.find(a.name);
		assert(it != impl->attribs.end());
		Attrib &b = it->second;
		glEnableVertexAttribArray(a.loc);
		glVertexAttribPointer(a.loc, b.size, b.type, b.normalized, b.stride, (GLvoid*)((char*)0 + b.offset));
	}


	impl->bindTarget(target);

	glBindBuffer(GL_ARRAY_BUFFER, impl->quadVerts);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, impl->quadIndices);

	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
}

struct QuadVert
{
	float x, y, z;
	float u, v;
};

QuadVert quad[] = {
	{ -1.0f, +1.0f, 0.0f, 0.0f, 1.0f },
	{ +1.0f, +1.0f, 0.0f, 1.0f, 1.0f },
	{ -1.0f, -1.0f, 0.0f, 0.0f, 0.0f },
	{ +1.0f, -1.0f, 0.0f, 1.0f, 0.0f },
};

uint16_t quadIndices[] = {
	0, 1, 2, 2, 1, 3,
};

Context *createContext()
{
	ContextImpl *ci = new ContextImpl();

	ci->backbufferWidth = 0;
	ci->backbufferHeight = 0;

	Attrib attribs[] = {
		{ 3, GL_FLOAT, false, 5 * sizeof(float), 0 * sizeof(float) },
		{ 2, GL_FLOAT, false, 5 * sizeof(float), 3 * sizeof(float) },
	};
	ci->attribs[std::string("a_position")] = attribs[0];
	ci->attribs[std::string("a_texCoord")] = attribs[1];

	glGenBuffers(1, &ci->quadVerts);
	glBindBuffer(GL_ARRAY_BUFFER, ci->quadVerts);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);

	glGenBuffers(1, &ci->quadIndices);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ci->quadIndices);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW);

	return ci;
}

void destroyContext(Context *c)
{
	ContextImpl *ci = (ContextImpl*)c;

	{
		std::vector<Pass*> leftPasses(ci->passes.begin(), ci->passes.end());
		for (auto pass : leftPasses)
			ci->destroyPass(pass);
		assert(ci->passes.empty());
	}

	{
		std::vector<Texture*> leftTextures(ci->textures.begin(), ci->textures.end());
		for (auto tex : leftTextures)
			ci->destroyTexture(tex);
		assert(ci->textures.empty());
	}

	glDeleteBuffers(1, &ci->quadVerts);
	glDeleteBuffers(1, &ci->quadIndices);

	delete ci;
}

};

