#version 300 es

in mediump vec3 v_normal;
in mediump vec2 v_texcoord;

struct Light
{
	mediump vec3 ambient;
	mediump vec3 directional;
	mediump vec3 direction;
};

uniform Light u_light;
uniform sampler2D u_tex;

out mediump vec4 frag_color;

void main()
{
	mediump vec4 albedo = texture(u_tex, v_texcoord);

	// magic values for the look I want
	mediump float directional_strength = 2.4;
	mediump float ambient_strength = 1.0;
	mediump float gamma = 1.1;

	// Directional
	mediump float d = dot(v_normal, u_light.direction);
	mediump float light_dot = max(0.0, d);
	mediump vec3 lighting = u_light.directional * light_dot * directional_strength;

	// Ambient
	lighting += u_light.ambient * ambient_strength;

	frag_color = albedo * vec4(lighting, 1.0);
	frag_color.rgb = pow(frag_color.rgb, vec3(1.0 / gamma));
}