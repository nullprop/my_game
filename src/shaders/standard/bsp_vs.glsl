/*================================================================
	* shaders/standard/bsp_vs.glsl
	*
	* Copyright (c) 2021 Lauri Räsänen
	* ================================

	BSP vertex shader.
=================================================================*/

#version 330 core

layout(location = 0) in vec3 a_pos;
layout(location = 1) in vec2 a_tex_coord;
layout(location = 2) in vec2 a_lm_coord;
layout(location = 3) in vec3 a_normal;
layout(location = 4) in vec4 a_color;

uniform mat4 u_proj;

out vec2 tex_coord;
out vec2 lm_coord;

void main()
{
	gl_Position = u_proj * vec4(a_pos, 1.0);
	tex_coord = a_tex_coord;
	lm_coord = a_lm_coord;
}