#version 460 core

in vec3 f_texcoord;

uniform samplerCube skybox;

out vec4 frag_color;

void main()
{
	frag_color = texture(skybox, f_texcoord);
}
