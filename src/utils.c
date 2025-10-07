/* See LICENSE for license details. */
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <cglm/cglm.h>
/* glew must be included first, then glfw */
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <SOIL2/SOIL2.h>

#include "utils.h"

struct dir_light dir_light = {
	{ 1.0f,-1.0f, 0.0f }, /* dir */
	{ 0.2f, 0.2f, 0.2f }, /* ambient */
	{ 1.0f, 1.0f, 1.0f }, /* diffuse */
	{ 1.0f, 1.0f, 1.0f }  /* specular */
};

struct pos_light pos_lights[] = {
	{
		{ 0.0f, 0.0f, 0.0f }, /* pos */
		{ 0.2f, 0.2f, 0.2f }, /* ambient */
		{ 1.0f, 1.0f, 1.0f }, /* diffuse */
		{ 1.0f, 1.0f, 1.0f }, /* specular */
		0.09f, /* linear */
		0.032f /* quadratic */
	},
};

struct state game = {
	{	/* cam */
		{ 0.0f, 0.0f, 3.0f },	/* pos */
		{ 0.0f, 0.0f,-1.0f },	/* front */
		{ 0.0f, 0.0f, 2.0f },	/* target */
		{ 0.0f, 1.0f, 0.0f },	/* up */
		2.0f,			/* speed */
		0.1f,			/* sensitivity */
		-PI / 2, 0.0f,	/* yaw and pitch */
		{ { 0 } }, { { 0 } }	/* view and projection matrices */
	},
	0,		/* input */
	0.0f, 0.0f,	/* delta_time and last_frame */
};

GLFWwindow *
initialize(void)
{
	glfwSetErrorCallback(error_callback);

	if (!glfwInit()) {
		glfwTerminate();
		errlog("couldn't initialize GLFW.");
		exit(1);
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, FULLNAME, NULL, NULL);
	if (!window) {
		glfwTerminate();
		errlog("GLFW window creation failed.");
		exit(1);
	}
	glfwMakeContextCurrent(window);

	GLenum err = glewInit();
	if (err != GLEW_OK) {
		glfwDestroyWindow(window);
		glfwTerminate();
		errlog("couldn't initialize GLEW.");
		exit(1);
	}
	glfwSwapInterval(1);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetKeyCallback(window, key_callback);
	glfwSetFramebufferSizeCallback(window, frame_buffer_size_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetWindowUserPointer(window, (void *) &game);

	glm_lookat(
		game.cam.pos,
		game.cam.target,
		game.cam.up,
		game.cam.view
	);

	glm_perspective(PI / 4, (float) WIDTH / HEIGHT, 0.05f, 128.0f, game.cam.projection);

	glEnable(GL_DEPTH_TEST);

	return window;
}

GLchar *
read_file(const char *path)
{
	FILE *fp = fopen(path, "rb");

	fseek(fp, 0L, SEEK_END);
	long size = ftell(fp);
	fseek(fp, 0L, SEEK_SET);

	GLchar *src = malloc((size + 1) * sizeof(GLchar));
	if (src == NULL)
		return NULL;

	for (long i = 0; i < size; i++)
		src[i] = getc(fp);
	src[size] = '\0';

	return src;
}

const GLuint
create_shader(const char *path, const GLenum type)
{
	GLchar *src = read_file(path);
	if (src == NULL) {
		errlog("couldn't read the %s file.", path);
		glfwTerminate();
		exit(1);
	}
	/*
	 * creating a const pointer so it can be passed as OpenGL expects it,
	 * but preserving the original to free it afterwards.
	 */
	const GLchar *source = src;

	const GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &source, NULL);
	glCompileShader(shader);
	free(src);

	int success;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		char info_log[512];
		glGetShaderInfoLog(shader, sizeof(info_log), NULL, info_log);
		fprintf(stderr, info_log);
		errlog("couldn't compile the %s shader.", path);
		glfwTerminate();
		exit(1);
	}

	return shader;
}

const GLuint
create_shader_program(const GLuint vs, const GLuint fs)
{
	const GLuint program = glCreateProgram();
	glAttachShader(program, vs);
	glAttachShader(program, fs);
	glLinkProgram(program);

	int success;
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success) {
		char info_log[512];
		glGetProgramInfoLog(program, sizeof(info_log), NULL, info_log);
		glfwTerminate();
		fprintf(stderr, info_log);
		errlog("couldn't link the shaders.");
		exit(1);
	}
	return program;
}

const GLuint
create_texture(const char *path)
{
	/*
	int width, height;
	unsigned char *img = SOIL_load_image(path, &width, &height, 0, SOIL_LOAD_RGBA);
	if (!img) return 0;

	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, img);
	glGenerateMipmap(GL_TEXTURE_2D);

	SOIL_free_image_data(img);

	return tex;
	*/
	return SOIL_load_OGL_texture(
		path,
		SOIL_LOAD_AUTO,
		SOIL_CREATE_NEW_ID,
		SOIL_FLAG_MIPMAPS |
		SOIL_FLAG_COMPRESS_TO_DXT
	);
}
const GLuint
create_texture_from_memory(const unsigned char *buffer, size_t size)
{
	return SOIL_load_OGL_texture_from_memory(
		buffer,
		size,
		SOIL_LOAD_RGBA,
		SOIL_CREATE_NEW_ID,
		SOIL_FLAG_MIPMAPS
	);
}

const GLuint
create_cubemap(const char *paths[6])
{
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	GLuint cubemap = SOIL_load_OGL_cubemap(
		paths[0], paths[1], paths[2],
		paths[3], paths[4], paths[5],
		SOIL_LOAD_RGB,
		SOIL_CREATE_NEW_ID,
		SOIL_FLAG_MIPMAPS |
		SOIL_FLAG_COMPRESS_TO_DXT
	);

	return cubemap;
}

void
move_camera(void)
{
	float speed = game.cam.speed * game.delta_time;
	if (game.input & DUCK) {
		speed /= 4;
	}
	else if (game.input & SPRINT) {
		speed *= 4;
	}

	vec3 scaled_front;
	if (game.input & BACKWARD) {
		glm_vec3_scale(game.cam.front, speed, scaled_front);
		glm_vec3_sub(game.cam.pos, scaled_front, game.cam.pos);
	}
	if (game.input & FORWARD) {
		glm_vec3_scale(game.cam.front, speed, scaled_front);
		glm_vec3_add(game.cam.pos, scaled_front, game.cam.pos);
	}
	vec3 cam_right;
	if (game.input & LEFT) {
		glm_cross(game.cam.front, game.cam.up, cam_right);
		glm_normalize(cam_right);
		glm_vec3_scale(cam_right, speed, cam_right);
		glm_vec3_sub(game.cam.pos, cam_right, game.cam.pos);
	}
	if (game.input & RIGHT) {
		glm_cross(game.cam.front, game.cam.up, cam_right);
		glm_normalize(cam_right);
		glm_vec3_scale(cam_right, speed, cam_right);
		glm_vec3_add(game.cam.pos, cam_right, game.cam.pos);
	}

	glm_vec3_add(game.cam.pos, game.cam.front, game.cam.target);

	glm_lookat(
		game.cam.pos,
		game.cam.target,
		game.cam.up,
		game.cam.view
	);
}

void
errlog(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	fprintf(stderr, BIN ": ");
	vfprintf(stderr, format, args);
	putc('\n', stderr);
	va_end(args);
}

void
error_callback(int error, const char *description)
{
	fprintf(stderr, "%s\n", description);
	errlog("GLFW detected an OpenGL error.");
}

void 
press(const int i, const int action)
{
	if (action == GLFW_PRESS)
		game.input |= i;
	else if (action == GLFW_RELEASE)
		game.input &= ~i;
}

void
mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
	static double last_x, last_y;
	static int regained = 1;

	int focused = glfwGetWindowAttrib(window, GLFW_FOCUSED);

	if (!focused) {
		regained = 1;
		return;
	}
	else if (focused && regained) {
		regained = 0;
		last_x = xpos;
		last_y = ypos;
	}

	float xoffset = xpos - last_x;
	float yoffset = last_y - ypos;
	last_x = xpos;
	last_y = ypos;

	float sensitivity = game.cam.sensitivity * game.delta_time;
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	game.cam.yaw += xoffset;
	game.cam.pitch += yoffset;
	if (game.cam.pitch > PI / 2 - 0.01) {
		game.cam.pitch = PI / 2 - 0.01;
	}
	else if (game.cam.pitch < -PI / 2 + 0.01) {
		game.cam.pitch = -PI / 2 + 0.01;
	}

	double cos_pitch = cos(game.cam.pitch);
	game.cam.front[0] = cos(game.cam.yaw) * cos_pitch;
	game.cam.front[1] = sin(game.cam.pitch);
	game.cam.front[2] = sin(game.cam.yaw) * cos_pitch;
	glm_normalize(game.cam.front);

	glm_vec3_add(game.cam.pos, game.cam.front, game.cam.target);

	glm_lookat(
		game.cam.pos,
		game.cam.target,
		game.cam.up,
		game.cam.view
	);
}

void
key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	switch (key) {
	case GLFW_KEY_A:
		press(LEFT, action);
		break;
	case GLFW_KEY_S:
		press(BACKWARD, action);
		break;
	case GLFW_KEY_W:
		press(FORWARD, action);
		break;
	case GLFW_KEY_D:
		press(RIGHT, action);
		break;
	case GLFW_KEY_LEFT_SHIFT:
		press(DUCK, action);
		break;
	case GLFW_KEY_LEFT_CONTROL:
		press(SPRINT, action);
		break;
	case GLFW_KEY_Q:
		glfwSetWindowShouldClose(window, GLFW_TRUE);
		break;
	}
}

void
frame_buffer_size_callback(GLFWwindow *window, int width, int height)
{
	int viewport_width = height * ((float) WIDTH / HEIGHT);
	int viewport_height = height;
	int viewport_x = (width - viewport_width) >> 1;
	int viewport_y = 0;

	glViewport(viewport_x, viewport_y, viewport_width, viewport_height);
}

struct skybox
create_skybox(const char *paths[6])
{
	float vertices[] = {
		-0.5f,-0.5f, 0.5f, 0.5f,-0.5f, 0.5f,
		 0.5f, 0.5f, 0.5f,-0.5f, 0.5f, 0.5f,
		-0.5f,-0.5f,-0.5f, 0.5f,-0.5f,-0.5f,
		 0.5f, 0.5f,-0.5f,-0.5f, 0.5f,-0.5f,
	};

	unsigned int elements[] = {
		0, 1, 2, 2, 3, 0, 5, 4, 7, 7, 6, 5,
		4, 0, 3, 3, 7, 4, 1, 5, 6, 6, 2, 1,
		3, 2, 6, 6, 7, 3, 4, 5, 1, 1, 0, 4,
	};

	struct skybox skybox;
	skybox.ID = create_cubemap(paths);
	skybox.n_indices = sizeof(elements);

	glGenVertexArrays(1, &skybox.VAO);
	glGenBuffers(1, &skybox.VBO);
	glGenBuffers(1, &skybox.EBO);

	glBindVertexArray(skybox.VAO);
	glBindBuffer(GL_ARRAY_BUFFER, skybox.VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skybox.EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), &elements, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *) 0);
	glEnableVertexAttribArray(0);

	glBindVertexArray(0);

	return skybox;
}

void
render_skybox(const struct skybox skybox, const GLuint shader_program)
{
	glDepthFunc(GL_LEQUAL);
	glUseProgram(shader_program);

	mat4 rotation_matrix;
	glm_mat4_identity(rotation_matrix);
	mat3 rotation_matrix3;
	glm_mat4_pick3(game.cam.view, rotation_matrix3);
	glm_mat4_ins3(rotation_matrix3, rotation_matrix);

	mat4 transformation_matrix;
	glm_mat4_mul(game.cam.projection, rotation_matrix, transformation_matrix);

	GLuint u_transformation = glGetUniformLocation(shader_program, "u_transformation");
	glUniformMatrix4fv(u_transformation, 1, GL_FALSE, *transformation_matrix);

	glBindVertexArray(skybox.VAO);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.ID);

	glDrawElements(GL_TRIANGLES, skybox.n_indices, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);

	glDepthFunc(GL_LESS);
}
