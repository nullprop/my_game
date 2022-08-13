#version 300 es

flat in int barrel_enabled;
in mediump vec3 uv;
in mediump vec2 uv_dot;

uniform sampler2D u_tex;

out mediump vec4 frag_color;

void main()
{
	if (barrel_enabled == 1)
	{
		mediump vec3 tex_coord = dot(uv_dot, uv_dot) * vec3(-0.5, -0.5, -1.0) + uv;
		frag_color = textureProj(u_tex, tex_coord);
	}
	else
	{
		frag_color = texture(u_tex, uv.xy);
	}	
}