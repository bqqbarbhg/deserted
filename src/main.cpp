#include "postprocess.h"
#include "sandbox.h"

#include <GL/glew.h>
#include <GL/GL.h>
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <vector>
#include <unordered_map>
#include <algorithm>

#include "../ext/stb_image_write.h"
#include "../ext/imgui.h"
#include "../ext/imgui_impl_glfw_gl3.h"

GLFWwindow *window;
void setup(pp::Context *ctx, sb::Sandbox *sb);
void loop(pp::Context *ctx, sb::Sandbox *sb);

int main(int argc, char **argv)
{
	glfwInit();
	window = glfwCreateWindow(1280, 720, "Sandbox", 0, 0);
	glfwMakeContextCurrent(window);

	glewInit();

	ImGui_ImplGlfwGL3_Init(window, true);

	pp::Context *ctx = pp::createContext();
	sb::Sandbox *sb = sb::createSandbox(ctx);

	setup(ctx, sb);

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		ImGui_ImplGlfwGL3_NewFrame();

		double time = glfwGetTime();
		sb->hotloadShaders();

		int w, h;
		glfwGetWindowSize(window, &w, &h);
		ctx->setBackbufferSize(w, h);

		double cx, cy;
		glfwGetCursorPos(window, &cx, &cy);

		float mouseVec[2] = { (float)(cx / w), 1.0f - (float)(cy / h) };
		ctx->setVector("mouse", mouseVec, 2);

		float timeVec[1] = { (float)time };
		ctx->setVector("time", timeVec, 1);

		loop(ctx, sb);

		GLenum error = glGetError();
		if (error)
			fprintf(stderr, "GL error: %d\n", error);

		ImGui::Render();

		glfwSwapBuffers(window);
	}

	sb::destroySandbox(sb);
	pp::destroyContext(ctx);

	return 0;
}

