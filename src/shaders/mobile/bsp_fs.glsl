#version 300 es

in mediump vec2 tex_coord;
in mediump vec2 lm_coord;

uniform sampler2D u_tex;
uniform sampler2D u_lm;

out mediump vec4 frag_color;

void main()
{
	mediump vec4 tex = texture(u_tex, tex_coord);
	mediump vec4 lm = texture(u_lm, lm_coord);

	// magic values for the look I want
	mediump float lm_strength = 2.8;
	mediump float gamma = 1.15;

	frag_color = tex * lm * lm_strength;
	frag_color.rgb = pow(frag_color.rgb, vec3(1.0/gamma));
	frag_color.a = 1.0;
}