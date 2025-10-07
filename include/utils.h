/* See LICENSE for license details. */

#define PI 3.14159265358979323846

enum {
	LEFT = 1,
	BACKWARD = 2,
	FORWARD = 4,
	RIGHT = 8,
	DUCK = 16,
	SPRINT = 32,
};

struct dir_light {
	vec3 dir;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

struct pos_light {
	vec3 pos;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	float linear;
	float quadratic;
};

struct camera {
	vec3 pos;
	vec3 front;
	vec3 target;
	vec3 up;
	float speed;
	float sensitivity;
	float yaw, pitch;
	mat4 view, projection;
};

struct state {
	struct camera cam;
	int input;
	float delta_time, last_frame;
};

struct skybox {
	GLuint ID, VAO, VBO, EBO;
	GLuint n_indices;
};

extern struct dir_light dir_light;
extern struct pos_light pos_lights[];
extern struct state game;

GLFWwindow *initialize(void);

GLchar *read_file(const char *path);

const GLuint create_shader(const char *path, const GLenum type);

const GLuint create_shader_program(const GLuint vs, const GLuint fs);

const GLuint create_texture(const char *path);

const GLuint create_texture_from_memory(const unsigned char *buffer, size_t size);

const GLuint create_cubemap(const char *paths[6]);

void move_camera(void);

void errlog(const char *format, ...);

void error_callback(int error, const char *description);

void press(const int i, const int action);

void mouse_callback(GLFWwindow *window, double xpos, double ypos);

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

void frame_buffer_size_callback(GLFWwindow *window, int width, int height);

struct skybox create_skybox(const char *paths[6]);

void render_skybox(const struct skybox skybox, const GLuint shader_program);
