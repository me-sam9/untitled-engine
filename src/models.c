/* See LICENSE for license details. */
#include <stdlib.h>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>
#include <cglm/cglm.h>
/* glew must be included first, then glfw */
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <SOIL2/SOIL2.h>

#include "utils.h"
#include "models.h"

extern struct state game;

GLuint
cgltf_load_texture(const cgltf_texture *tex, const char *path)
{
	if (tex == NULL) return 0;

	cgltf_image *image = tex->image;

	if (image == NULL) return 0;

	cgltf_buffer_view *image_view = image->buffer_view;

	if (image->uri != NULL) {

		char *dir = strrchr(path, '/') + 1;
		size_t dir_length = dir - path;

		char *uri = image->uri;
		size_t uri_length = strlen(uri);

		size_t size = dir_length + uri_length;
		char *fullpath = malloc(size);
		fullpath[size - 1] = '\0';

		strncpy(fullpath, path, dir_length);
		strcpy(fullpath + dir_length, uri);

		return create_texture(fullpath);
	}
	else if (image_view != NULL) {

		cgltf_buffer *image_buffer = image_view->buffer;
		unsigned char *image_buffer_cpy = malloc(image_view->size);

		size_t n = image_view->offset;
		int stride = image_view->stride ? image_view->stride : 1;

		for (size_t i = 0; i < image_view->size; i++) {
			image_buffer_cpy[i] = ((unsigned char *) image_buffer->data)[n];
			n += stride;
		}

		return create_texture_from_memory(image_buffer_cpy, n);
	}
	return 0;
}

struct model
load_model(const char *path)
{
	struct model model = { 0 };

	cgltf_options options = { 0 };
	cgltf_data *data = NULL;
	cgltf_result result = cgltf_parse_file(&options, path, &data);
	if (result != cgltf_result_success) {
		errlog("failed to load the %s model.", path);
		exit(1);
	}
	result = cgltf_load_buffers(&options, data, path);
	if (result != cgltf_result_success) {
		errlog("failed to load the buffers of the %s model.", path);
		cgltf_free(data);
		exit(1);
	}

	for (size_t i = 0; i < data->meshes_count; i++) {
		model.n_meshes += data->meshes[i].primitives_count;
	}

	model.meshes = calloc(model.n_meshes, sizeof(struct mesh));
	struct mesh *meshes = model.meshes;
	if (meshes == NULL) {
		errlog("failed to load the %s model.", path);
		exit(1);
	}

	size_t mesh_index = 0;
	for (size_t mi = 0; mi < data->meshes_count; mi++) {
		for (size_t pi = 0; pi < data->meshes[mi].primitives_count; pi++) {
			struct mesh *mesh = meshes + mesh_index;
			cgltf_primitive primitive = data->meshes[mi].primitives[pi];

			/* bind VAO */
			glGenVertexArrays(1, &mesh->VAO);
			glBindVertexArray(mesh->VAO);

			/* load mesh indices */
			cgltf_accessor *indices_accessor = primitive.indices;
			if (indices_accessor->type != cgltf_type_scalar) {
				errlog(
					"invalid index array in the %s model "
	    				"(not buffer view of scalars)", path
				);
				exit(1);
			}

			switch (indices_accessor->component_type) {
				case cgltf_component_type_r_16u:
					mesh->index_type = GL_UNSIGNED_SHORT;
					break;
				case cgltf_component_type_r_32u:
					mesh->index_type = GL_UNSIGNED_INT;
					break;
				default:
					mesh->index_type = GL_UNSIGNED_BYTE;
					break;
			}

			cgltf_buffer_view *indices_view = indices_accessor->buffer_view;
			cgltf_buffer *indices_buffer = indices_view->buffer;

			glGenBuffers(1, &mesh->EBO);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->EBO);
			glBufferData(
				GL_ELEMENT_ARRAY_BUFFER,
				indices_view->size,
				(char *) indices_buffer->data + indices_view->offset,
				GL_STATIC_DRAW
			);

			mesh->n_indices = indices_accessor->count;

			/* load buffers */
			char *pos_buffer = NULL;
			char *nor_buffer = NULL;
			char *uvs_buffer = NULL;
			char *col_buffer = NULL;
			size_t pos_stride = 0;
			size_t nor_stride = 0;
			size_t uvs_stride = 0;
			size_t col_stride = 0;

			for (size_t ai = 0; ai < primitive.attributes_count; ai++) {
				cgltf_attribute attribute = primitive.attributes[ai];
				cgltf_accessor *attr_accessor = attribute.data;
				cgltf_buffer_view *attr_view = attr_accessor->buffer_view;
				cgltf_buffer *attr_buffer = attr_view->buffer;

				/*
				if (attr_accessor->component_type != cgltf_component_type_r_32f) {
					errlog(
						"failed to load the attributes #%zu, "
						"type %d in the %s model (the component "
						"type of the accesor is not float)",
						ai, attribute.type, path
					);
					exit(1);
				}
				*/

				switch (attribute.type) {
				case cgltf_attribute_type_position:
					if (attr_accessor->type != cgltf_type_vec3) {
						errlog(
							"failed to load the pos attribute #%zu, "
							"type %d in the %s model (not a vec3)",
							ai, attribute.type, path
						);
						exit(1);
					}

					pos_buffer = (char *) attr_buffer->data + attr_view->offset;
					pos_stride = attr_accessor->stride;
					mesh->n_vertices = attr_accessor->count;
					break;
				case cgltf_attribute_type_color:
					if (attr_accessor->type != cgltf_type_vec4) {
						errlog(
							"failed to load the color attribute #%zu, "
							"type %d in the %s model (not a vec4)",
							ai, attribute.type, path
	     					);
						exit(1);
					}

					col_buffer = (char *) attr_buffer->data + attr_view->offset;
					col_stride = attr_accessor->stride;
					break;
				case cgltf_attribute_type_texcoord:
					if (attr_accessor->type != cgltf_type_vec2) {
						errlog(
							"failed to load the textcoord attribute #%zu, "
							"type %d in the %s model (not a vec2)",
							ai, attribute.type, path
						);
						exit(1);
					}

					uvs_buffer = (char *) attr_buffer->data + attr_view->offset;
					uvs_stride = attr_accessor->stride;
					break;
				case cgltf_attribute_type_normal:
					if (attr_accessor->type != cgltf_type_vec3) {
						errlog(
							"failed to load the normal attribute #%zu, "
							"type %d in the %s model (not a vec3)",
							ai, attribute.type, path
						);
						exit(1);
					}

					nor_buffer = (char *) attr_buffer->data + attr_view->offset;
					nor_stride = attr_accessor->stride;
					break;
				default:
					errlog(
						"ignoring attribute #%zu, "
						"type %d in the %s model (not supported)",
	     					ai, attribute.type, path
					);
					continue;
				}
			}

			if (pos_buffer == NULL) {
				errlog("there are no positions for the vertices.");
				exit(1);
			}

			mesh->vertices = calloc(mesh->n_vertices, sizeof(struct vertex));
			for (size_t vi = 0; vi < mesh->n_vertices; vi++) {
				struct vertex *vertex = &mesh->vertices[vi];

				vertex->pos[0] = *((float *) (pos_buffer + pos_stride * vi));
				vertex->pos[1] = *((float *) (pos_buffer + pos_stride * vi + sizeof(float)));
				vertex->pos[2] = *((float *) (pos_buffer + pos_stride * vi + 2 * sizeof(float)));

				if (nor_buffer != NULL) {
					vertex->nor[0] = *((float *) (nor_buffer + nor_stride * vi));
					vertex->nor[1] = *((float *) (nor_buffer + nor_stride * vi + sizeof(float)));
					vertex->nor[2] = *((float *) (nor_buffer + nor_stride * vi + 2 * sizeof(float)));
				}
				if (uvs_buffer != NULL) {
					vertex->uvs[0] = *((float *) (uvs_buffer + uvs_stride * vi));
					vertex->uvs[1] = *((float *) (uvs_buffer + uvs_stride * vi + sizeof(float)));
				}
				if (col_buffer != NULL) {
					vertex->col[0] = *((float *) (col_buffer + col_stride * vi));
					vertex->col[1] = *((float *) (col_buffer + col_stride * vi + sizeof(float)));
					vertex->col[2] = *((float *) (col_buffer + col_stride * vi + 2 * sizeof(float)));
					vertex->col[3] = *((float *) (col_buffer + col_stride * vi + 3 * sizeof(float)));
				}
			}

			glGenBuffers(1, &mesh->VBO);
			glBindBuffer(GL_ARRAY_BUFFER, mesh->VBO);
			glBufferData(GL_ARRAY_BUFFER, mesh->n_vertices * sizeof(struct vertex), mesh->vertices, GL_STATIC_DRAW);

			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(struct vertex),
			(void *) offsetof(struct vertex, pos));
			glEnableVertexAttribArray(0); /* position */

			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(struct vertex),
			(void *) offsetof(struct vertex, nor));
			glEnableVertexAttribArray(1); /* normal */

			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(struct vertex),
			(void *) offsetof(struct vertex, uvs));
			glEnableVertexAttribArray(2); /* textcoord */

			glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(struct vertex),
			(void *) offsetof(struct vertex, col));
			glEnableVertexAttribArray(3); /* color */

			/* load diffuse and specular textures */
			if (primitive.material == NULL) {
				mesh->diffuse = SOIL_load_OGL_texture(
					"img/err.bmp",
					SOIL_LOAD_RGB,
					SOIL_CREATE_NEW_ID,
					SOIL_FLAG_MIPMAPS
				);
				mesh->culling = 0;
			}
			else {
				cgltf_material *material = primitive.material;
				cgltf_texture *tex = material->pbr_metallic_roughness.base_color_texture.texture;

				mesh->diffuse = cgltf_load_texture(tex, path);
				mesh->culling = !material->double_sided;
			}
			mesh_index++;
		}
	}

	glBindVertexArray(0);
	cgltf_free(data);
	return model;
}

void
render_model(const struct model model, const GLuint shader_program, mat4 model_matrix)
{
	mat4 normal_matrix4;
	mat3 normal_matrix;
	glm_mat4_inv(model_matrix, normal_matrix4);
	glm_mat4_pick3t(normal_matrix4, normal_matrix);

	mat4 transformation_matrix;
	glm_mat4_mul(game.cam.view,       model_matrix, transformation_matrix);
	glm_mat4_mul(game.cam.projection, transformation_matrix, transformation_matrix);

	float material_shininess = 128.0f;

	GLuint u_model          = glGetUniformLocation(shader_program, "u_model");
	GLuint u_normal         = glGetUniformLocation(shader_program, "u_normal");
	GLuint u_transformation = glGetUniformLocation(shader_program, "u_transformation");

	GLuint u_material_diffuse   = glGetUniformLocation(shader_program, "u_material.diffuse");
	GLuint u_material_specular  = glGetUniformLocation(shader_program, "u_material.specular");
	GLuint u_material_shininess = glGetUniformLocation(shader_program, "u_material.shininess");

	GLuint u_dir_light_direction = glGetUniformLocation(shader_program, "u_dir_light.direction");
	GLuint u_dir_light_ambient   = glGetUniformLocation(shader_program, "u_dir_light.ambient");
	GLuint u_dir_light_diffuse   = glGetUniformLocation(shader_program, "u_dir_light.diffuse");
	GLuint u_dir_light_specular  = glGetUniformLocation(shader_program, "u_dir_light.specular");

	GLuint u_pos_light_position  = glGetUniformLocation(shader_program, "u_pos_lights[0].position");
	GLuint u_pos_light_ambient   = glGetUniformLocation(shader_program, "u_pos_lights[0].ambient");
	GLuint u_pos_light_diffuse   = glGetUniformLocation(shader_program, "u_pos_lights[0].diffuse");
	GLuint u_pos_light_specular  = glGetUniformLocation(shader_program, "u_pos_lights[0].specular");
	GLuint u_pos_light_linear    = glGetUniformLocation(shader_program, "u_pos_lights[0].linear");
	GLuint u_pos_light_quadratic = glGetUniformLocation(shader_program, "u_pos_lights[0].quadratic");

	GLuint u_camera_position = glGetUniformLocation(shader_program, "u_camera_position");

	glUseProgram(shader_program);

	glUniformMatrix4fv(u_model,          1, GL_FALSE, *model_matrix);
	glUniformMatrix3fv(u_normal,         1, GL_FALSE, *normal_matrix);
	glUniformMatrix4fv(u_transformation, 1, GL_FALSE, *transformation_matrix);

	glUniform1i(u_material_diffuse, 0);
	glUniform1i(u_material_specular, 1);
	glUniform1f(u_material_shininess, material_shininess);

	glUniform3fv(u_dir_light_direction, 1, dir_light.dir);
	glUniform3fv(u_dir_light_ambient,   1, dir_light.ambient);
	glUniform3fv(u_dir_light_diffuse,   1, dir_light.diffuse);
	glUniform3fv(u_dir_light_specular,  1, dir_light.specular);

	glUniform3fv(u_pos_light_position, 1, pos_lights[0].pos);
	glUniform3fv(u_pos_light_ambient,  1, pos_lights[0].ambient);
	glUniform3fv(u_pos_light_diffuse,  1, pos_lights[0].diffuse);
	glUniform3fv(u_pos_light_specular, 1, pos_lights[0].specular);
	glUniform1f(u_pos_light_linear,       pos_lights[0].linear);
	glUniform1f(u_pos_light_quadratic,    pos_lights[0].quadratic);

	glUniform3fv(u_camera_position, 1, game.cam.pos);

	for (int i = 0; i < model.n_meshes; i++) {
		glBindVertexArray(model.meshes[i].VAO);

		glActiveTexture(GL_TEXTURE0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
		glBindTexture(GL_TEXTURE_2D, model.meshes[i].diffuse);

		/*
		glActiveTexture(GL_TEXTURE1);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
		glBindTexture(GL_TEXTURE_2D, model.meshes[i].specular);
		*/
	
		if (model.meshes[i].culling) {
			glEnable(GL_CULL_FACE);
		}
		else {
			glDisable(GL_CULL_FACE);
		}

		glDrawElements(GL_TRIANGLES, model.meshes[i].n_indices, model.meshes[i].index_type, 0);
	}

	glBindVertexArray(0);
}
