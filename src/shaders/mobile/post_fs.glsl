#version 300 es

flat in int barrel_enabled;
in mediump vec3 uv;
in mediump vec2 uv_dot;

uniform sampler2D u_tex;
uniform sampler2D u_tex_vm;

out mediump vec4 frag_color;

void main()
{
	mediump vec4 color1;
	mediump vec4 color2;

	if (barrel_enabled == 1)
	{
		mediump vec3 tex_coord = dot(uv_dot, uv_dot) * vec3(-0.5, -0.5, -1.0) + uv;
		color1 = textureProj(u_tex, tex_coord);
		color2 = textureProj(u_tex, tex_coord);
	}
	else
	{
		color1 = texture(u_tex, uv.xy);
		color2 = texture(u_tex_vm, uv.xy);
	}

	frag_color = mix(color1, color2, color2.a);
}