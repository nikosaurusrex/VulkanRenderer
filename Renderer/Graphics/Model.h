#ifndef MODEL_H
#define MODEL_H

#include "Common.h"
#include "Vulkan/VulkanRenderer.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

struct DirectionalLight {
    glm::vec4 ambient;
    glm::vec4 diffuse;
    glm::vec3 dir;
    f32 _padding;
};

struct PointLight {
    glm::vec4 ambient;
    glm::vec4 diffuse;
    glm::vec3 pos;
    f32 _padding;
};

struct PointLights {
    u32 count;
    glm::vec3 _padding;
    PointLight lights[10];
};

struct Transformation {
    glm::mat4 projection;
    glm::mat4 view;
    glm::mat4 model;
};

struct Material {
    glm::vec4 ambient;
    glm::vec4 diffuse;
    glm::vec4 specular;
    f32 shininess;
    glm::vec3 _padding;
};

struct Mesh {
    StorageBuffer *vertices_buffer = 0;
    IndexBuffer *index_buffer = 0;
    u32 material_index = 0;
};

struct MeshData {
    glm::mat4 model_matrix;
    u32 material_index;
};

struct Model {
    StorageBuffer *materials_buffer = 0;
	array<Mesh *> meshes;
    glm::mat4 transformation;

	Model();
	~Model();
};

struct ModelImporter {
    static Model *Load(const char *path, VkCommandPool command_pool);
};

#endif