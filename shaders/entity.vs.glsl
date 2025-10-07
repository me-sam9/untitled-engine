#version 460 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texcoord;
layout (location = 3) in vec4 color;

uniform mat4 u_model;
uniform mat3 u_normal;
uniform mat4 u_transformation;

out vec3 f_fragment_position;
out vec3 f_normal;
out vec2 f_texcoord;
out vec4 f_color;

void
main()
{
	f_fragment_position = vec3(u_model * vec4(position, 1.0f));
	f_normal = u_normal * normal;
	f_texcoord = texcoord;
	f_color = color;

	gl_Position = u_transformation * vec4(position, 1.0f);
}
