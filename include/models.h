/* See LICENSE for license details. */

struct vertex {
	vec3 pos;
	vec3 nor;
	vec2 uvs;
	vec4 col;
};

struct mesh {
	struct vertex *vertices;
	size_t n_vertices;
	size_t n_indices;
	GLuint VAO;
	GLuint VBO;
	GLuint EBO;
	GLuint diffuse;
	GLuint specular;
	GLenum index_type;
	int culling;
};

struct model {
	struct mesh *meshes;
	size_t n_meshes;
};

struct model load_model(const char *path);

void render_model(const struct model model, const GLuint shader_program, mat4 model_matrix);
