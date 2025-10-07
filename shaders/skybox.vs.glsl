#version 460 core

layout (location = 0) in vec3 pos;

uniform mat4 u_transformation;

out vec3 f_texcoord;

void
main()
{
	f_texcoord = pos;
	gl_Position = u_transformation * vec4(pos, 1.0f);
	gl_Position = gl_Position.xyww;
}
