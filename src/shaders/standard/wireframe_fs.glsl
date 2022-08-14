/*================================================================
	* shaders/standard/wireframe_fs.glsl
	*
	* Copyright (c) 2022 Lauri Räsänen
	* ================================

	Basic wireframe shader.
=================================================================*/

#version 330 core

uniform vec4 u_color;

out vec4 frag_color;

void main()
{
	frag_color = u_color;
}