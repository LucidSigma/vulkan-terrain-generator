#version 450

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_colour;

layout (location = 0) flat out vec3 v_colour;

layout (std140, push_constant) uniform Model
{
	mat4 model;
} u_Model;

layout (std140, set = 0, binding = 0) uniform VP
{
	mat4 view;
	mat4 projection;
} u_ViewProjection;

void main()
{
	v_colour = in_colour;
	gl_Position = u_ViewProjection.projection * u_ViewProjection.view * u_Model.model * vec4(in_position, 1.0);
}