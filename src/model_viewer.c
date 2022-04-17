/*================================================================
	* model_viewer.c
	*
	* Copyright (c) 2021 Lauri Räsänen
	* ================================

	The main entry point of my_game model viewer.
=================================================================*/

#define GS_IMPL
#include <gs/gs.h>
#define GS_IMMEDIATE_DRAW_IMPL
#include <gs/util/gs_idraw.h>
#define GS_GUI_IMPL
#include <gs/util/gs_gui.h>

#include "bsp/bsp_loader.h"
#include "bsp/bsp_map.h"
#include "entities/player.h"
#include "graphics/model_manager.h"
#include "graphics/renderer.h"
#include "graphics/texture_manager.h"
#include "graphics/ui_manager.h"
#include "util/config.h"
#include "util/console.h"
#include "util/transform.h"

gs_camera_t *camera	    = NULL;
float32_t camera_yaw	    = 0;
float32_t camera_pitch	    = 0;
gs_vqs *model_transform	    = NULL;
char *model_path	    = NULL;
uint32_t model_id	    = 0;
mg_renderable_t *renderable = NULL;
uint32_t animation_index    = 0;
uint32_t animation_count    = 0;
bool32_t animation_paused   = false;
bool32_t animation_loop	    = true;
uint32_t text_fps	    = GS_SLOT_ARRAY_INVALID_HANDLE;
uint32_t text_animation	    = GS_SLOT_ARRAY_INVALID_HANDLE;
uint32_t text_anim_fps	    = GS_SLOT_ARRAY_INVALID_HANDLE;
uint32_t text_anim_frame    = GS_SLOT_ARRAY_INVALID_HANDLE;
uint32_t text_anim_pause    = GS_SLOT_ARRAY_INVALID_HANDLE;
uint32_t text_anim_loop	    = GS_SLOT_ARRAY_INVALID_HANDLE;

void app_init()
{
	camera			   = gs_malloc_init(gs_camera_t);
	*camera			   = gs_camera_perspective();
	camera->near_plane	   = 0.1f;
	camera->far_plane	   = 1000.0f;
	camera->transform.position = gs_v3(0, -50.0f, 0);

	// Init managers, free in app_shutdown if adding here
	mg_texture_manager_init();
	mg_model_manager_init();
	mg_renderer_init(gs_platform_main_window());
	mg_ui_manager_init();

	g_renderer->cam = camera;

	// Lock mouse at start by default
	gs_platform_lock_mouse(gs_platform_main_window(), true);
	if (glfwRawMouseMotionSupported())
	{
		glfwSetInputMode(gs_platform_raw_window_handle(gs_platform_main_window()), GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
	}

	mg_model_t *model = mg_model_manager_find(model_path);
	if (model == NULL)
	{
		_mg_model_manager_load(model_path, "basic");
		model = mg_model_manager_find(model_path);
	}
	model_transform		  = gs_malloc_init(gs_vqs);
	model_transform->position = gs_v3(0, 0, 0);
	model_transform->rotation = gs_quat_from_euler(0.0f, 0.0f, 0.0f);
	model_transform->scale	  = gs_v3(1.0f, 1.0f, 1.0f);
	model_id		  = mg_renderer_create_renderable(*model, model_transform);
	renderable		  = mg_renderer_get_renderable(model_id);
	animation_count		  = gs_dyn_array_size(renderable->model.data->animations);

	char tmp[256];

	int32_t text_y = 5;
	int32_t text_h = 15;
	text_fps       = mg_ui_manager_add_text("FPS: 0", gs_v2(5, text_y), 32);

	int32_t num_surfaces = renderable->model.data->header.num_surfaces;
	int32_t num_verts    = 0;
	int32_t num_tris     = 0;
	for (size_t i = 0; i < num_surfaces; i++)
	{
		md3_surface_t surf = renderable->model.data->surfaces[i];
		num_verts += surf.num_verts;
		num_tris += surf.num_tris;
	}

	text_y += text_h;
	sprintf(tmp, "Model: %s", model_path);
	mg_ui_manager_add_text(tmp, gs_v2(5, (text_y += text_h)), 0);
	sprintf(tmp, "Surfaces: %d", num_surfaces);
	mg_ui_manager_add_text(tmp, gs_v2(10, (text_y += text_h)), 32);
	sprintf(tmp, "Verts: %d", num_verts);
	mg_ui_manager_add_text(tmp, gs_v2(10, (text_y += text_h)), 32);
	sprintf(tmp, "Tris: %d", num_tris);
	mg_ui_manager_add_text(tmp, gs_v2(10, (text_y += text_h)), 32);

	text_y += text_h;
	text_animation	= mg_ui_manager_add_text("Animation: None", gs_v2(5, (text_y += text_h)), 64);
	text_anim_fps	= mg_ui_manager_add_text("Anim FPS: 0", gs_v2(10, (text_y += text_h)), 32);
	text_anim_frame = mg_ui_manager_add_text("Frame: 0 / 0", gs_v2(10, (text_y += text_h)), 32);
	sprintf(tmp, "Pause: %d", animation_paused);
	text_anim_pause = mg_ui_manager_add_text(tmp, gs_v2(10, (text_y += text_h)), 0);
	sprintf(tmp, "Loop: %d", animation_loop);
	text_anim_loop = mg_ui_manager_add_text(tmp, gs_v2(10, (text_y += text_h)), 0);

	text_y += text_h;
	mg_ui_manager_add_text("Controls:", gs_v2(5, (text_y += text_h)), 0);
	mg_ui_manager_add_text("WASD - Move", gs_v2(10, (text_y += text_h)), 0);
	mg_ui_manager_add_text("Mouse - Look", gs_v2(10, (text_y += text_h)), 0);
	mg_ui_manager_add_text("P - Pause", gs_v2(10, (text_y += text_h)), 0);
	mg_ui_manager_add_text("L - Loop", gs_v2(10, (text_y += text_h)), 0);
	mg_ui_manager_add_text("R - Restart animation", gs_v2(10, (text_y += text_h)), 0);
	mg_ui_manager_add_text("Up/Down - Next/Prev animation", gs_v2(10, (text_y += text_h)), 0);
	mg_ui_manager_add_text("Left/Right - Skip frame", gs_v2(10, (text_y += text_h)), 0);
}

void app_update()
{
	gs_platform_t *platform = gs_subsystem(platform);
	float delta_time	= platform->time.delta;
	double plat_time	= gs_platform_elapsed_time();
	char tmp[64];

	// If click, then lock again (in case lost)
	if (gs_platform_mouse_pressed(GS_MOUSE_LBUTTON) && !gs_platform_mouse_locked() && !g_ui_manager->show_cursor)
	{
		gs_platform_lock_mouse(gs_platform_main_window(), true);
		if (glfwRawMouseMotionSupported())
		{
			glfwSetInputMode(gs_platform_raw_window_handle(gs_platform_main_window()), GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
		}
	}

	if (gs_platform_key_down(GS_KEYCODE_ESC))
	{
		gs_quit();
	}

	gs_vec3 wish_move = gs_v3(0, 0, 0);
	gs_vec2 dp	  = gs_vec2_scale(gs_platform_mouse_deltav(), 2.0f * 0.022f);

	camera_pitch		   = gs_clamp(camera_pitch - dp.y, -90.0f, 90.0f);
	camera_yaw		   = fmodf(camera_yaw - dp.x, 360.0f);
	camera->transform.rotation = gs_quat_mul(
		gs_quat_angle_axis(gs_deg2rad(camera_yaw), MG_AXIS_UP),
		gs_quat_angle_axis(gs_deg2rad(camera_pitch), MG_AXIS_RIGHT));

	if (gs_platform_key_down(GS_KEYCODE_W))
		wish_move = gs_vec3_add(wish_move, mg_get_forward(camera->transform.rotation));
	if (gs_platform_key_down(GS_KEYCODE_S))
		wish_move = gs_vec3_add(wish_move, mg_get_backward(camera->transform.rotation));
	if (gs_platform_key_down(GS_KEYCODE_D))
		wish_move = gs_vec3_add(wish_move, mg_get_right(camera->transform.rotation));
	if (gs_platform_key_down(GS_KEYCODE_A))
		wish_move = gs_vec3_add(wish_move, mg_get_left(camera->transform.rotation));
	if (gs_platform_key_down(GS_KEYCODE_SPACE))
		wish_move = gs_vec3_add(wish_move, MG_AXIS_UP);
	if (gs_platform_key_down(GS_KEYCODE_LEFT_CONTROL))
		wish_move = gs_vec3_add(wish_move, MG_AXIS_DOWN);

	wish_move		   = gs_vec3_norm(wish_move);
	camera->transform.position = gs_vec3_add(camera->transform.position, gs_vec3_scale(wish_move, 200.0f * delta_time));

	if (gs_platform_key_pressed(GS_KEYCODE_F2)) g_ui_manager->console_open = !g_ui_manager->console_open;
	if (gs_platform_key_pressed(GS_KEYCODE_F3)) g_ui_manager->debug_open = !g_ui_manager->debug_open;

	// Play next animation
	if (gs_platform_key_pressed(GS_KEYCODE_UP) && animation_count > 0)
	{
		animation_index++;
		if (animation_index >= animation_count)
			animation_index = 0;

		mg_renderer_play_animation(model_id, renderable->model.data->animations[animation_index].name);
		if (renderable->current_animation != NULL)
		{
			renderable->current_animation->loop = animation_loop;
			if (animation_paused)
				renderable->prev_frame_time = DBL_MAX;

			sprintf(tmp, "Animation: %s", renderable->current_animation->name);
			mg_ui_manager_update_text(text_animation, tmp);
			sprintf(tmp, "Anim FPS: %d", renderable->current_animation->fps);
			mg_ui_manager_update_text(text_anim_fps, tmp);
		}
	}

	// Play previous animation
	if (gs_platform_key_pressed(GS_KEYCODE_DOWN) && animation_count > 0)
	{
		animation_index--;
		// underflow
		if (animation_index >= animation_count)
			animation_index = animation_count - 1;

		mg_renderer_play_animation(model_id, renderable->model.data->animations[animation_index].name);
		if (renderable->current_animation != NULL)
		{
			renderable->current_animation->loop = animation_loop;
			if (animation_paused)
				renderable->prev_frame_time = DBL_MAX;

			sprintf(tmp, "Animation: %s", renderable->current_animation->name);
			mg_ui_manager_update_text(text_animation, tmp);
			sprintf(tmp, "Anim FPS: %d", renderable->current_animation->fps);
			mg_ui_manager_update_text(text_anim_fps, tmp);
		}
	}

	// Toggle pause
	if (gs_platform_key_pressed(GS_KEYCODE_P))
	{
		animation_paused = !animation_paused;
		// Set prev_frame_time to future so animation wont progress
		if (animation_paused)
			renderable->prev_frame_time = DBL_MAX;
		else
			renderable->prev_frame_time = plat_time;

		sprintf(tmp, "Pause: %d", animation_paused);
		mg_ui_manager_update_text(text_anim_pause, tmp);
	}

	// Restart animation
	if (gs_platform_key_pressed(GS_KEYCODE_R))
	{
		if (renderable->current_animation != NULL)
		{
			renderable->frame	    = renderable->current_animation->first_frame;
			renderable->prev_frame_time = plat_time;
		}
	}

	// Toggle loop
	if (gs_platform_key_pressed(GS_KEYCODE_L))
	{
		animation_loop = !animation_loop;
		if (renderable->current_animation != NULL)
		{
			renderable->current_animation->loop = animation_loop;
		}
		sprintf(tmp, "Loop: %d", animation_loop);
		mg_ui_manager_update_text(text_anim_loop, tmp);
	}

	// Frame skip forwards
	if (gs_platform_key_pressed(GS_KEYCODE_RIGHT) && renderable->current_animation != NULL)
	{
		renderable->frame++;
		if (renderable->frame >= renderable->current_animation->first_frame + renderable->current_animation->num_frames)
		{
			if (animation_loop)
				renderable->frame = renderable->current_animation->first_frame;
			else
				renderable->frame = renderable->current_animation->first_frame + renderable->current_animation->num_frames - 1;
		}
	}

	// Frame skip backwards
	if (gs_platform_key_pressed(GS_KEYCODE_LEFT) && renderable->current_animation != NULL)
	{
		renderable->frame--;
		if (renderable->frame < renderable->current_animation->first_frame)
		{
			if (animation_loop)
				renderable->frame = renderable->current_animation->first_frame + renderable->current_animation->num_frames - 1;
			else
				renderable->frame = renderable->current_animation->first_frame;
		}
	}

	sprintf(tmp, "FPS: %d", (int)gs_round(1.0f / delta_time));
	mg_ui_manager_update_text(text_fps, tmp);

	sprintf(tmp, "Frame: %d / %d",
		renderable->current_animation != NULL ? renderable->frame - renderable->current_animation->first_frame + 1 : 0,
		renderable->current_animation != NULL ? renderable->current_animation->num_frames : 0);
	mg_ui_manager_update_text(text_anim_frame, tmp);

	mg_renderer_update();
}

void app_shutdown()
{
	mg_renderer_free();
	mg_ui_manager_free();
	mg_model_manager_free();
	mg_texture_manager_free();
	gs_free(camera);
	gs_free(model_transform);
	gs_free(model_path);
}

gs_app_desc_t gs_main(int32_t argc, char **argv)
{
	if (argc < 2)
	{
		mg_println("Missing model path argument");
		gs_quit();
		return;
	}

	size_t sz  = gs_string_length(argv[1]) + 1;
	model_path = gs_malloc(sz);
	memcpy(model_path, argv[1], sz);

	mg_config_init();

	return (gs_app_desc_t){
		.init	       = app_init,
		.update	       = app_update,
		.shutdown      = app_shutdown,
		.window_title  = "ModelViewer",
		.window_flags  = 0,
		.window_width  = 800,
		.window_height = 600,
		.enable_vsync  = 1,
		.frame_rate    = 60,
	};
}