/*================================================================
	* shaders/standard/basic_unlit_fs.glsl
	*
	* Copyright (c) 2021 Lauri Räsänen
	* ================================

	Basic fragment shader with texture.
=================================================================*/

#version 330 core

in vec2 v_texcoord;

uniform sampler2D u_tex;

out vec4 frag_color;

void main()
{
	frag_color = texture(u_tex, v_texcoord);
};