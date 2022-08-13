#version 300 es

in mediump vec2 v_texcoord;

uniform sampler2D u_tex;

out mediump vec4 frag_color;

void main()
{
	frag_color = texture(u_tex, v_texcoord);
}