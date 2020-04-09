#version 450

layout (location = 0) in vec3 v_colour;
layout (location = 1) in vec3 v_normal;
layout (location = 2) in vec3 v_fragmentPosition;

layout (location = 0) out vec4 out_colour;

struct Light {
	vec3 position;
	vec3 colour;

	float ambientStrength;
} g_light;

void main()
{
	g_light.position = vec3(0.0, 128.0, 0.0);
	g_light.colour = vec3(1.0, 1.0, 1.0);
	g_light.ambientStrength = 0.3;

	const vec3 ambientLight = g_light.ambientStrength * g_light.colour;

	const vec3 lightDirection = normalize(g_light.position - v_fragmentPosition);
	const float diffuseStrength = max(dot(v_normal, lightDirection), 0.0);
	const vec3 diffuseLight = diffuseStrength * g_light.colour;

	out_colour = vec4((ambientLight + diffuseLight) * v_colour, 1.0);
}