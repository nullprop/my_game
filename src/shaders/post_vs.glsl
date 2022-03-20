/*================================================================
	* shaders/post_vs.glsl
	*
	* Copyright (c) 2021 Lauri Räsänen
	* ================================

	Post-processing effects.

	Barrel distortion based on:
	https://www.decarpentier.nl/lens-distortion
=================================================================*/

#version 330 core

layout(location = 0) in vec2 a_pos;

uniform int u_barrel_enabled;
uniform float u_barrel_strength;
uniform float u_barrel_height;
uniform float u_barrel_aspect;
uniform float u_barrel_cyl_ratio;

flat out int barrel_enabled;
out vec3 uv;
out vec2 uv_dot;

void main()
{
	gl_Position = vec4(a_pos, 0, 1.0);

	barrel_enabled = u_barrel_enabled;
	if (u_barrel_enabled == 1)
	{
		float scaled_height = u_barrel_strength * u_barrel_height;
		float cyl_aspect = u_barrel_aspect * u_barrel_cyl_ratio;
		float apsect_diag_sq = u_barrel_aspect * u_barrel_aspect + 1.0;
		float diag_sq = scaled_height * scaled_height * apsect_diag_sq;
		
		float z = 0.5 * sqrt(diag_sq + 1.0) + 0.5;
		float ny = (z - 1.0) / (cyl_aspect * cyl_aspect + 1.0);

		uv_dot = sqrt(ny) * vec2(cyl_aspect, 1.0) * a_pos;
		uv = vec3(0.5, 0.5, 1.0) * z + vec3(-0.5, -0.5, 0.0);
		uv.xy += vec2(max(0, a_pos.x), max(0, a_pos.y));
	}
	else
	{
		uv.xy = vec2(max(0, a_pos.x), max(0, a_pos.y));
	}
}