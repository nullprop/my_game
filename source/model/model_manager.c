/*================================================================
    * model/model_manager.c
    *
    * Copyright (c) 2021 Lauri Räsänen
    * ================================

    ...
=================================================================*/

#include <assimp/cimport.h>
#include <assimp/postprocess.h>

#include "model_manager.h"

mg_model_manager_t *g_model_manager;

void mg_model_manager_init()
{
    // Allocate
    g_model_manager = gs_malloc_init(mg_model_manager_t);
    g_model_manager->models = gs_dyn_array_new(mg_model_t);

    // Test
    _mg_model_manager_load("models/test.gltf");
}

void mg_model_manager_free()
{
#ifdef MG_USE_ASSIMP
    for (size_t i = 0; i < gs_dyn_array_size(g_model_manager->models); i++)
    {
        gs_free(g_model_manager->models[i].data.vertices);
        gs_free(g_model_manager->models[i].data.indices);
        g_model_manager->models[i].data.vertices = NULL;
        g_model_manager->models[i].data.indices = NULL;
    }
#endif
    gs_dyn_array_free(g_model_manager->models);

    gs_free(g_model_manager);
    g_model_manager = NULL;
}

mg_model_t *mg_model_manager_find(char *filename)
{
    for (size_t i = 0; i < gs_dyn_array_size(g_model_manager->models); i++)
    {
        if (strcmp(filename, g_model_manager->models[i].filename) == 0)
        {
            return &g_model_manager->models[i];
        }
    }

    gs_println("WARN: mg_model_manager_find invalid model %s", filename);
    return NULL;
}

#ifdef MG_USE_ASSIMP
void _mg_model_manager_load(char *filename)
{
    const struct aiScene *scene = aiImportFile(
        filename,
        aiProcess_CalcTangentSpace |
            aiProcess_Triangulate |
            aiProcess_JoinIdenticalVertices |
            aiProcess_SortByPType);

    if (scene == NULL)
    {
        gs_println("ERR: _mg_model_manager_load fail: %s", filename);
        gs_println(aiGetErrorString());
        return;
    }

    if (scene->mNumMeshes != 1)
    {
        gs_println("ERR: _mg_model_manager_load fail: %s", filename);
        gs_println("invalid number of meshes: %zu", scene->mNumMeshes);
        aiReleaseImport(scene);
        return;
    }

    struct aiMesh *mesh = scene->mMeshes[0];

    gs_dyn_array(gs_vec3) vertices = gs_dyn_array_new(gs_vec3);
    gs_dyn_array_reserve(vertices, mesh->mNumVertices);
    gs_dyn_array_head(vertices)->size = mesh->mNumVertices;

    gs_dyn_array(uint16_t) indices = gs_dyn_array_new(uint16_t);
    gs_dyn_array_reserve(indices, mesh->mNumFaces * 3);
    gs_dyn_array_head(indices)->size = mesh->mNumFaces * 3;

    for (size_t i = 0; i < mesh->mNumVertices; i++)
    {
        vertices[i] = gs_v3((float)mesh->mVertices[i].x, (float)mesh->mVertices[i].y, (float)mesh->mVertices[i].z);
    }

    for (size_t i = 0; i < mesh->mNumFaces; i++)
    {
        if (mesh->mFaces[i].mNumIndices != 3)
        {
            gs_println("ERR: _mg_model_manager_load fail: %s", filename);
            gs_println("face % zu invalid number of indices: %zu", i, mesh->mFaces[i].mNumIndices);
            aiReleaseImport(scene);
            return;
        }

        for (size_t j = 0; j < 3; j++)
        {
            indices[i * 3 + j] = (uint16_t)(mesh->mFaces[i].mIndices[j]);
        }
    }

    mg_model_t model = (mg_model_t){
        .filename = filename,
        .data = (mg_model_data_t){
            .vertices = vertices,
            .indices = indices,
        },
    };

    aiReleaseImport(scene);

    gs_dyn_array_push(g_model_manager->models, model);

    gs_println("Model: Loaded %s, verts: %d, indices: %d", filename, gs_dyn_array_size(vertices), gs_dyn_array_size(indices));
}
#else
void _mg_model_manager_load(char *filename)
{
    if (!gs_util_file_exists(filename))
    {
        gs_println("ERR: _mg_model_manager_load file not found %s", filename);
        return;
    }

    gs_gfxt_mesh_import_options_t options = gs_default_val();
    gs_gfxt_mesh_t mesh = gs_gfxt_mesh_create_from_file(filename, &options);
    mg_model_t model = {
        .filename = filename,
        .data = mesh,
    };

    gs_dyn_array_push(g_model_manager->models, model);

    gs_println("Model: Loaded %s", filename);
}
#endif