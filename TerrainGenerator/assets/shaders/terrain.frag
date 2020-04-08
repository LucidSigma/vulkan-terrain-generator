#version 450

layout (location = 0) flat in vec3 v_colour;

layout (location = 0) out vec4 out_colour;

void main()
{
	out_colour = vec4(v_colour, 1.0);
}