/*================================================================
	* shaders/standard/wireframe_vs.glsl
	*
	* Copyright (c) 2022 Lauri Räsänen
	* ================================

	Basic wireframe shader.
=================================================================*/

#version 330 core

layout(location = 0) in vec3 a_pos;

uniform mat4 u_proj;

void main()
{
	gl_Position = u_proj * vec4(a_pos, 1.0);
}