#version 300 es

layout(location = 0) in mediump vec3 a_pos;

uniform mediump mat4 u_proj;
uniform mediump mat4 u_view;

void main()
{
	gl_Position = u_proj * u_view * vec4(a_pos, 1.0);
}