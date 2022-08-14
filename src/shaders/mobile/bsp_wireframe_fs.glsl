#version 300 es

uniform mediump vec4 u_color;

out mediump vec4 frag_color;

void main()
{
	frag_color = u_color;
}