#version 460 core

in vec3 f_normal;
in vec3 f_fragment_position;
in vec2 f_texcoord;
in vec4 f_color;

uniform vec3 u_camera_position;

struct material {
	sampler2D diffuse;
	sampler2D specular;
	float shininess;
};

uniform material u_material;

struct dir_light {
	vec3 direction;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

uniform dir_light u_dir_light;

struct pos_light {
	vec3 position;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;

	float linear;
	float quadratic;
};

#define POS_LIGHTS 4
uniform pos_light u_pos_lights[POS_LIGHTS];

out vec4 frag_color;

vec4 dir_light_contrib(vec4 diffuse_tex, vec4 specular_tex, vec3 normal, vec3 view_dir);
vec4 pos_light_contrib(vec4 diffuse_tex, vec4 specular_tex, vec3 normal, vec3 view_dir, pos_light u_pos_light);

void
main()
{
	vec4 diffuse_tex = texture(u_material.diffuse, f_texcoord);
	if (diffuse_tex.a < 0.8f) {
		discard;
	}
	vec4 specular_tex = texture(u_material.specular, f_texcoord);
	vec3 normal = normalize(f_normal);
	vec3 view_dir = normalize(u_camera_position - f_fragment_position);

	frag_color = vec4(0.0f);
	frag_color += dir_light_contrib(diffuse_tex, specular_tex, normal, view_dir);
	for (int i = 0; i < POS_LIGHTS; i++) {
		frag_color += pos_light_contrib(diffuse_tex, specular_tex, normal, view_dir, u_pos_lights[i]);
	}
}

vec4
dir_light_contrib(vec4 diffuse_tex, vec4 specular_tex, vec3 normal, vec3 view_dir)
{
	/* ambient */
	vec4 ambient = diffuse_tex * vec4(u_dir_light.ambient, 1.0f);

	/* diffuse */
	vec3 light_direction = normalize(-u_dir_light.direction);
	float diff = max(dot(normal, light_direction), 0.0f);
	vec4 diffuse = diff * diffuse_tex * vec4(u_dir_light.diffuse, 1.0f);
	
	/* specular */
	vec3 reflect_direction = reflect(-light_direction, normal);
	float spec = pow(max(dot(view_dir, reflect_direction), 0.0f), u_material.shininess);
	vec4 specular = spec * specular_tex * vec4(u_dir_light.specular, 1.0f);

	return ambient + diffuse + specular;
}

vec4
pos_light_contrib(vec4 diffuse_tex, vec4 specular_tex, vec3 normal, vec3 view_dir, pos_light u_pos_light)
{
	/* ambient */
	vec4 ambient = diffuse_tex * vec4(u_pos_light.ambient, 1.0f);

	/* diffuse */
	vec3 light_direction = normalize(u_pos_light.position - f_fragment_position);
	float diff = max(dot(normal, light_direction), 0.0f);
	vec4 diffuse = diff * diffuse_tex * vec4(u_pos_light.diffuse, 1.0f);
	
	/* specular */
	vec3 reflect_direction = reflect(-light_direction, normal);
	float spec = pow(max(dot(view_dir, reflect_direction), 0.0f), u_material.shininess);
	vec4 specular = spec * specular_tex * vec4(u_pos_light.specular, 1.0f);

	/* distance attenuation */
	float dist = length(u_pos_light.position - f_fragment_position);
	float attenuation = 1.0f / (1.0f + u_pos_light.linear * dist + u_pos_light.quadratic * dist * dist);
	ambient *= attenuation;
	diffuse *= attenuation;
	specular *= attenuation;

	return ambient + diffuse + specular;
}
