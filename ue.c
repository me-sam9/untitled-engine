/* See LICENSE for license details. */
#include <math.h>

#include <cglm/cglm.h>
/* glew must be included first, then glfw */
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <SOIL2/SOIL2.h>

#include "utils.h"
#include "models.h"

int
main(void)
{
	GLFWwindow *window = initialize();

	const char *faces[] = {
		"img/skybox/right.jpg",
		"img/skybox/left.jpg",
		"img/skybox/top.jpg",
		"img/skybox/bottom.jpg",
		"img/skybox/front.jpg",
		"img/skybox/back.jpg"
	};

	struct skybox skybox = create_skybox(faces);

	const GLuint entity_vs = create_shader("shaders/entity.vs.glsl", GL_VERTEX_SHADER);
	const GLuint entity_fs = create_shader("shaders/entity.fs.glsl", GL_FRAGMENT_SHADER);
	const GLuint light_vs = create_shader("shaders/light.vs.glsl", GL_VERTEX_SHADER);
	const GLuint light_fs = create_shader("shaders/light.fs.glsl", GL_FRAGMENT_SHADER);
	const GLuint skybox_vs = create_shader("shaders/skybox.vs.glsl", GL_VERTEX_SHADER);
	const GLuint skybox_fs = create_shader("shaders/skybox.fs.glsl", GL_FRAGMENT_SHADER);

	const GLuint entity_shader_program = create_shader_program(entity_vs, entity_fs);
	const GLuint light_shader_program = create_shader_program(light_vs, light_fs);
	const GLuint skybox_shader_program = create_shader_program(skybox_vs, skybox_fs);

	glDeleteShader(entity_vs);
	glDeleteShader(entity_fs);
	glDeleteShader(light_vs);
	glDeleteShader(light_fs);
	glDeleteShader(skybox_vs);
	glDeleteShader(skybox_fs);

	const struct model map = load_model("mod/map/map.glb");
	const struct model marble = load_model("mod/marble/marble_bust_01_4k.gltf");
	const struct model light = load_model("mod/sphere/sphere.glb");

	mat4 map_model_matrix, light_model_matrix, marble_model_matrix;

	glm_mat4_identity(map_model_matrix);
	glm_scale(map_model_matrix, (vec3) { 1.5f, 1.5f, 1.5f });
	glm_translate(map_model_matrix, (vec3) { 0.0f, -0.4f, 0.0f });

	glm_mat4_identity(marble_model_matrix);
	glm_mat4_identity(light_model_matrix);

	while (!glfwWindowShouldClose(window)) {
		float current_frame = glfwGetTime();
		game.delta_time = current_frame - game.last_frame;
		game.last_frame = current_frame;

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		if (game.input) {
			move_camera();
		}

		float radius = 1.0f;
		float light_x = sin(current_frame) * radius;
		float light_z = cos(current_frame) * radius;

		glm_vec3_copy((vec3) { light_x, light_x * light_x, light_z }, pos_lights[0].pos);
		glm_mat4_identity(light_model_matrix);
		glm_translate(light_model_matrix, pos_lights[0].pos);
		glm_scale(light_model_matrix, (vec3) { 0.1f, 0.1f, 0.1f });

		render_model(map, entity_shader_program, map_model_matrix);
		render_model(marble, entity_shader_program, marble_model_matrix);
		render_model(light, light_shader_program, light_model_matrix);

		render_skybox(skybox, skybox_shader_program);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();
	return 0;
}
