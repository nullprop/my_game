/*================================================================
	* shaders/standard/basic_unlit_vs.glsl
	*
	* Copyright (c) 2021 Lauri Räsänen
	* ================================

	Basic vertex shader.
=================================================================*/

#version 330 core

layout(location = 0) in vec3 a_pos;
layout(location = 1) in vec2 a_texcoord;

uniform mat4 u_proj;
uniform mat4 u_view;

out vec2 v_texcoord;

void main()
{
	v_texcoord = a_texcoord;
	gl_Position = u_proj * u_view * vec4(a_pos, 1.0);
}