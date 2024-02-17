#version 450

#extension GL_EXT_shader_explicit_arithmetic_types: require

layout (location=0) out vec4 frag_color;

struct Vertex {
    float px, py, pz;
    uint8_t nx, ny, nz, nw;
    float tu, tv;
};

struct Material {
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    float shininess; 
};

struct DirectionalLight {
	vec4 ambient;
	vec4 diffuse;
	vec3 dir;
};

struct PointLight {
	vec4 ambient;
	vec4 diffuse;
	vec3 pos;
};

layout(binding=0) readonly buffer SceneData {
    mat4 projection_matrix;
    mat4 view_matrix;
    DirectionalLight dir_light;
    uint8_t num_point_lights;
    PointLight point_lights[10];
};

layout(binding=1) readonly buffer VertexData {
    Vertex vertices[];
};

layout(binding=2) readonly buffer MaterialData {
    Material materials[];
};

layout(push_constant) uniform MeshData {
    mat4 model_matrix;
    uint material_index;
};

vec3 CalculateDirLight(DirectionalLight light, Material mat, vec3 normal) {
	vec3 ray = normalize(light.dir);
	
    vec4 ambient = light.ambient * mat.ambient;
    float diff = max(dot(normal, ray), 0.0);
    vec4 diffuse = light.diffuse * (diff * mat.diffuse);

	return (ambient + diffuse).xyz;
}

vec3 CalculatePointLight(PointLight light, Material mat, vec3 normal, vec3 frag_pos) {
	vec3 ray = normalize(light.pos - frag_pos);
	
	vec4 ambient = light.ambient * mat.ambient;
	float diff = max(dot(normal, ray), 0.0);
	vec4 diffuse = light.diffuse * (diff * mat.diffuse);

	return (ambient + diffuse).xyz;
}

void main() {
    Vertex v = vertices[gl_VertexIndex];
    Material m = materials[material_index];

    vec4 position = vec4(v.px, v.py, v.pz, 1.0);
    vec4 normal = vec4(vec3(v.nx, v.ny, v.nz) / 127.0 - 1.0, 1.0);
    vec2 tex_coord = vec2(v.tu, v.tv);

    vec4 world_pos = model_matrix * position;
	vec3 norm = normalize(mat3(transpose(inverse(model_matrix))) * normal.xyz);

    vec3 result = CalculateDirLight(dir_light, m, norm);

	for (int i = 0; i < num_point_lights; i++) {
		result += CalculatePointLight(point_lights[i], m, norm, world_pos.xyz);
	}

	gl_Position = projection_matrix * view_matrix * world_pos;
    frag_color = vec4(result, 1.0);
}
