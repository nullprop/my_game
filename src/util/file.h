/*================================================================
	* util/file.h
	*
	* Copyright (c) 2022 Lauri Räsänen
	* ================================

	Filesystem utils.
=================================================================*/

#ifndef MG_FILE_H
#define MG_FILE_H

#include <gs/gs.h>

#ifdef __ANDROID__
#include <android/asset_manager.h>
#endif // __ANDROID__

#ifdef __ANDROID__

static inline void *mg_android_get_asset_buffer(const char *filepath)
{
	gs_android_t *android = (gs_android_t *)gs_app()->android.activity;
	gs_assert(android);

	AAsset *asset = AAssetManager_open(android->asset_manager, filepath, AASSET_MODE_BUFFER);
	void *buf     = AAsset_getBuffer(asset);
	AAsset_close(asset);

	return buf;
}

static inline gs_dyn_array(char *) mg_android_get_asset_dir_files(const char *dirpath)
{
	gs_android_t *android = (gs_android_t *)gs_app()->android.activity;
	gs_assert(android);

	AAssetDir *dir		   = AAssetManager_openDir(android->asset_manager, dirpath);
	gs_dyn_array(char *) files = gs_dyn_array_new(char *);

	char *next = AAssetDir_getNextFileName(dir);
	size_t sz;

	while (next != NULL)
	{
		sz	  = strlen(next) + 1;
		char *tmp = gs_malloc(sz);
		memset(tmp, '\0', sz);
		strcat(tmp, next);
		gs_dyn_array_push(files, tmp);
		next = AAssetDir_getNextFileName(dir);
	}

	AAssetDir_close(dir);
	return files;
}

#endif // __ANDROID__

static inline void mg_unpack_assets()
{
#ifdef __ANDROID__
	// const char *[] folders = {
	// 	"cfg",
	// 	"fonts",
	// 	"maps",
	// 	"models",
	// 	"shaders",
	// 	"sound",
	// 	"textures",
	// };

	gs_dyn_array(char *) files = mg_android_get_asset_dir_files("");
	for (size_t i = 0; i < gs_dyn_array_size(files); i++)
	{
		mg_println(files[i]);
		gs_free(files[i]);
	}
	gs_dyn_array_free(files);

#endif // __ANDROID__
}

#endif // MG_FILE_H