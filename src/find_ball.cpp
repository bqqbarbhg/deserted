#include "postprocess.h"
#include "sandbox.h"

#include "../ext/imgui.h"

sb::Pass *p_FindEdges;
sb::Pass *p_Blur;
sb::Pass *p_Flip;
sb::Pass *p_Ball;
pp::Texture *t_Reprojected;
pp::Texture *t_Target[16];

void setup(pp::Context *ctx, sb::Sandbox *sb)
{
	p_FindEdges = sb->loadPass("shader/test_vertex.glsl", "shader/ball/find_edges.glsl"); 
	p_Blur = sb->loadPass("shader/test_vertex.glsl", "shader/ball/blur.glsl"); 
	p_Flip = sb->loadPass("shader/test_vertex.glsl", "shader/ball/flip.glsl"); 
	p_Ball = sb->loadPass("shader/test_vertex.glsl", "shader/ball/ball_kernel.glsl"); 

	t_Reprojected = sb->loadImage("image/reproj.png");
	t_Target[0] = t_Reprojected;
	for (int i = 1; i < 16; i++)
		t_Target[i] = ctx->createRenderTarget(1280, 720);
}

void loop(pp::Context *ctx, sb::Sandbox *sb)
{
	static int flipTarget = 3;
	ImGui::RadioButton("Reprojected", &flipTarget, 0);
	ImGui::RadioButton("Blur", &flipTarget, 1);
	ImGui::RadioButton("Find edges", &flipTarget, 2);
	ImGui::RadioButton("Ball kernel", &flipTarget, 3);

	static float radius = 3.0f;
	ImGui::SliderFloat("Radius", &radius, 0.0f, 20.0f);

	static float multiplier = 40.0f;
	ImGui::SliderFloat("Edge multiplier", &multiplier, 0.0f, 60.0f);

	ctx->clear(NULL, 0.0f, 0.0f, 0.0f, 1.0f);

	ctx->setTexture("tex", t_Reprojected);
	sb->renderPass(p_Blur, t_Target[1]);

	ctx->setVector("multiplier", &multiplier, 1);
	ctx->setTexture("tex", t_Target[1]);
	sb->renderPass(p_FindEdges, t_Target[2]);

	ctx->setVector("radius", &radius, 1);
	ctx->setTexture("tex", t_Target[2]);
	sb->renderPass(p_Ball, t_Target[3]);

	ctx->setTexture("tex", t_Target[flipTarget]);
	sb->renderPass(p_Flip, NULL);
}

