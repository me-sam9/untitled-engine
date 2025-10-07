#version 460 core

layout (location = 0) in vec3 position;

uniform mat4 u_transformation;

void
main()
{
	gl_Position = u_transformation * vec4(position, 1.0f);
}
