#include "Model.h"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

#include "Common.h"

Model::Model() {
}

Model::~Model() {
	for (Mesh *mesh : meshes) {
        mesh->vertices_buffer->Destroy();
		mesh->index_buffer->Destroy();
		delete mesh->vertices_buffer;
		delete mesh->index_buffer;
        delete mesh;
	}

	materials_buffer->Destroy();
	delete materials_buffer;
}

Model *ModelImporter::Load(const char *path, VkCommandPool command_pool) {
    Assimp::Importer importer;

	const u32 import_flags =
		aiProcess_CalcTangentSpace |
		aiProcess_Triangulate |
		aiProcess_JoinIdenticalVertices |
		aiProcess_GenNormals |
		aiProcess_GenUVCoords |
		aiProcess_SortByPType |
		aiProcess_OptimizeMeshes |
		aiProcess_GlobalScale |
		aiProcess_ValidateDataStructure;

    const aiScene *scene = importer.ReadFile(path, import_flags);

	if (!scene) {
		LogFatal("Failed to load model: %s", importer.GetErrorString());
	}

	Model *model = new Model();
    
    if (scene->HasMaterials()) {
        Material *materials = new Material[scene->mNumMaterials];

        for (u64 i = 0; i < scene->mNumMaterials; ++i) {
            aiMaterial *aiMat = scene->mMaterials[i];
            
            Material *mat = &materials[i];

            aiColor3D ambient, diffuse, specular;
            if (aiMat->Get(AI_MATKEY_COLOR_AMBIENT, ambient) == AI_SUCCESS) {
                mat->ambient = glm::vec4(ambient.r, ambient.g, ambient.b, 1.0f);
            }
            if (aiMat->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse) == AI_SUCCESS) {
                mat->diffuse = glm::vec4(diffuse.r, diffuse.g, diffuse.b, 1.0f);
            }
            if (aiMat->Get(AI_MATKEY_COLOR_SPECULAR, specular) == AI_SUCCESS) {
                mat->specular = glm::vec4(specular.r, specular.g, specular.b, 1.0f);
            }
        }

		model->materials_buffer = new StorageBuffer();
		model->materials_buffer->Create(materials, scene->mNumMaterials * sizeof(Material));

		delete[] materials;
    }

    model->meshes.resize(scene->mNumMeshes);
	for (int i = 0; i < scene->mNumMeshes; ++i) {
		aiMesh *ai_mesh = scene->mMeshes[i];

		u32 vertices_count = ai_mesh->mNumVertices;
		Vertex *vertices = new Vertex[vertices_count];

		u32 num_indices = ai_mesh->mNumFaces * 3;
		u32 *indices = new u32[num_indices];

		aiVector3D zero_vector(0.0f);
		for (u32 i = 0; i < ai_mesh->mNumVertices; ++i) {
			aiVector3D pos = ai_mesh->mVertices[i];
			aiVector3D tex_coords = ai_mesh->HasTextureCoords(0) ? ai_mesh->mTextureCoords[0][i] : zero_vector;
			aiVector3D normal = ai_mesh->mNormals[i];

            Vertex *vertex = &vertices[i];
            vertex->position = glm::vec<3, f32>(pos.x, pos.y, pos.z);
            vertex->normal = glm::vec<4, u8>(
                normal.x * 127.0f + 127.0f,
                normal.y * 127.0f + 127.0f,
                normal.z * 127.0f + 127.0f,
                1
            );
            vertex->tex_coord = glm::vec<2, f32>(tex_coords.x, tex_coords.y);
        }

		for (u32 i = 0; i < ai_mesh->mNumFaces; ++i) {
			aiFace face = ai_mesh->mFaces[i];
			indices[i * 3 + 0] = face.mIndices[0];
			indices[i * 3 + 1] = face.mIndices[1];
			indices[i * 3 + 2] = face.mIndices[2];
		}

        StorageBuffer *storage_buffer = new StorageBuffer();
        storage_buffer->Create((void *) &vertices[0], ai_mesh->mNumVertices * sizeof(Vertex));

        IndexBuffer *index_buffer = new IndexBuffer();
        index_buffer->Create((u32 *) &indices[0], num_indices, command_pool);
		
		Mesh *mesh = new Mesh;
		mesh->material_index	= ai_mesh->mMaterialIndex;
		mesh->vertices_buffer   = storage_buffer;
		mesh->index_buffer		= index_buffer;
		model->meshes[i]		= mesh;

		delete[] vertices;
		delete[] indices;
	}

    return model;
}