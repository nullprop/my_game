/*================================================================
    * entities/player.c
    *
    * Copyright (c) 2021 Lauri Räsänen
    * ================================

    ...
=================================================================*/

#include "player.h"
#include "../bsp/bsp_trace.h"
#include "../util/math.h"
#include "../util/transform.h"

mg_player_t *mg_player_new()
{
    mg_player_t *player = gs_malloc_init(mg_player_t);
    *player = (mg_player_t){
        .transform = gs_vqs_default(),
        .camera = {
            .cam = {
                .aspect_ratio = 4 / 3,
                .far_plane = 3000.0f,
                .fov = 110.0f,
                .near_plane = 0.1f,
                .proj_type = GS_PROJECTION_TYPE_PERSPECTIVE,
            },
        },
        .health = 100,
        .eye_pos = gs_v3(0, 0, MG_PLAYER_HEIGHT - MG_PLAYER_EYE_OFFSET),
        .mins = gs_v3(-MG_PLAYER_HALF_WIDTH, -MG_PLAYER_HALF_WIDTH, 0),
        .maxs = gs_v3(MG_PLAYER_HALF_WIDTH, MG_PLAYER_HALF_WIDTH, MG_PLAYER_HEIGHT),
    };

    _mg_player_camera_update(player);

    return player;
}

void mg_player_free(mg_player_t *player)
{
    gs_free(player);
    player = NULL;
}

void mg_player_update(mg_player_t *player)
{
    gs_platform_t *platform = gs_engine_subsystem(platform);
    float dt = platform->time.delta;

    _mg_player_check_floor(player);
    _mg_player_get_input(player, dt);

    // Handle jump and gravity
    if (player->grounded)
    {
        if (player->wish_jump)
        {
            player->velocity.z = MG_PLAYER_JUMP_SPEED;
            player->grounded = false;
        }
        else
        {
            player->velocity.z = 0;
        }
    }
    else
    {
        player->velocity = gs_vec3_add(player->velocity, gs_vec3_scale(MG_AXIS_DOWN, 800.0f * dt));
    }

    // Crouching
    if (player->wish_crouch)
    {
        _mg_player_crouch(player, dt);
    }
    else
    {
        _mg_player_uncrouch(player, dt);
    }

    // Update velocity
    _mg_player_friction(player, dt);

    if (player->grounded)
    {
        _mg_player_accelerate(player, dt, player->crouched ? MG_PLAYER_CROUCH_MOVE_SPEED : MG_PLAYER_MOVE_SPEED, MG_PLAYER_ACCEL, MG_PLAYER_MAX_VEL);
    }
    else
    {
        _mg_player_accelerate(player, dt, MG_PLAYER_AIR_MOVE_SPEED, MG_PLAYER_AIR_ACCEL, MG_PLAYER_MAX_VEL);
    }

    // Move
    _mg_player_slidemove(player, dt);

    _mg_player_camera_update(player);
}

void _mg_player_check_floor(mg_player_t *player)
{
    bsp_trace_t *trace = &(bsp_trace_t){.map = player->map};
    bsp_trace_box(
        trace,
        player->transform.position,
        gs_vec3_add(player->transform.position, gs_vec3_scale(MG_AXIS_DOWN, 2.0f)),
        player->mins,
        player->maxs);

    if (trace->fraction < 1.0f && trace->normal.z > 0.7f)
    {
        player->grounded = true;
        player->ground_normal = trace->normal;
    }
    else
    {
        player->grounded = false;
    }
}

void _mg_player_get_input(mg_player_t *player, float delta_time)
{
    player->wish_move = gs_v3(0, 0, 0);
    player->wish_jump = false;
    player->wish_crouch = false;

    gs_vec2 dp = gs_vec2_scale(gs_platform_mouse_deltav(), 1.8f * 0.022f);

    if (gs_platform_key_down(GS_KEYCODE_UP))
        dp.y -= 150.0f * delta_time;
    if (gs_platform_key_down(GS_KEYCODE_DOWN))
        dp.y += 150.0f * delta_time;
    if (gs_platform_key_down(GS_KEYCODE_RIGHT))
        dp.x += 150.0f * delta_time;
    if (gs_platform_key_down(GS_KEYCODE_LEFT))
        dp.x -= 150.0f * delta_time;

    // Rotate
    player->camera.pitch = gs_clamp(player->camera.pitch + dp.y, -90.0f, 90.0f);
    player->yaw = fmodf(player->yaw - dp.x, 360.0f);
    player->transform.rotation = gs_quat_angle_axis(gs_deg2rad(player->yaw), MG_AXIS_UP);

    if (gs_platform_key_down(GS_KEYCODE_W))
        player->wish_move = gs_vec3_add(player->wish_move, mg_get_forward(player->transform.rotation));
    if (gs_platform_key_down(GS_KEYCODE_S))
        player->wish_move = gs_vec3_add(player->wish_move, mg_get_backward(player->transform.rotation));
    if (gs_platform_key_down(GS_KEYCODE_D))
        player->wish_move = gs_vec3_add(player->wish_move, mg_get_right(player->transform.rotation));
    if (gs_platform_key_down(GS_KEYCODE_A))
        player->wish_move = gs_vec3_add(player->wish_move, mg_get_left(player->transform.rotation));

    player->wish_move.z = 0;
    player->wish_move = gs_vec3_norm(player->wish_move);

    if (gs_platform_key_down(GS_KEYCODE_SPACE))
        player->wish_jump = true;
    if (gs_platform_key_down(GS_KEYCODE_LEFT_CONTROL))
        player->wish_crouch = true;
}

void _mg_player_camera_update(mg_player_t *player)
{
    player->camera.cam.transform = gs_vqs_absolute_transform(
        &(gs_vqs){
            .position = player->eye_pos,
            .rotation = gs_quat_mul(
                gs_quat_angle_axis(gs_deg2rad(-player->camera.pitch), MG_AXIS_RIGHT),
                gs_quat_angle_axis(gs_deg2rad(player->camera.roll), MG_AXIS_FORWARD)),
            .scale = gs_v3(1.0f, 1.0f, 1.0f),
        },
        &player->transform);
}

void _mg_player_friction(mg_player_t *player, float delta_time)
{
    if (player->grounded == false) return;

    float vel2 = gs_vec3_len2(player->velocity);
    if (vel2 < GS_EPSILON)
    {
        player->velocity = gs_v3(0, 0, 0);
        return;
    }

    float vel = sqrtf(vel2);
    float loss = fmaxf(vel, MG_PLAYER_STOP_SPEED) * MG_PLAYER_FRICTION * delta_time;
    float fraction = fmaxf(0, vel - loss) / vel;

    player->velocity = gs_vec3_scale(player->velocity, fraction);
}

void _mg_player_accelerate(mg_player_t *player, float delta_time, float move_speed, float acceleration, float max_vel)
{
    // Velocity projected on to wished direction,
    // positive if in the same direction.
    float proj_vel = gs_vec3_dot(player->velocity, player->wish_move);

    // The max amount to change by,
    // won't accelerate past move_speed.
    float change = move_speed - proj_vel;
    if (change <= 0) return;

    // The actual acceleration
    float accel = acceleration * move_speed * delta_time;

    // Clamp to max change
    if (accel > change) accel = change;

    player->velocity = gs_vec3_add(player->velocity, gs_vec3_scale(player->wish_move, accel));

    // Clamp to max vel per axis
    player->velocity.x = fminf(MG_PLAYER_MAX_VEL, fmaxf(player->velocity.x, -MG_PLAYER_MAX_VEL));
    player->velocity.y = fminf(MG_PLAYER_MAX_VEL, fmaxf(player->velocity.y, -MG_PLAYER_MAX_VEL));
    player->velocity.z = fminf(MG_PLAYER_MAX_VEL, fmaxf(player->velocity.z, -MG_PLAYER_MAX_VEL));
}

void _mg_player_crouch(mg_player_t *player, float delta_time)
{
    // Can always crouch
    player->crouched = true;
    player->maxs.z = MG_PLAYER_CROUCH_HEIGHT;
    player->eye_pos.z = MG_PLAYER_CROUCH_HEIGHT - MG_PLAYER_EYE_OFFSET;
}

void _mg_player_uncrouch(mg_player_t *player, float delta_time)
{
    // Only uncrouch if room
    bsp_trace_t *trace = &(bsp_trace_t){.map = player->map};
    bsp_trace_box(
        trace,
        player->transform.position,
        gs_vec3_add(player->transform.position, gs_vec3_scale(mg_get_up(player->transform.rotation), MG_PLAYER_HEIGHT - MG_PLAYER_CROUCH_HEIGHT)),
        player->mins, player->maxs);

    if (trace->fraction == 1.0f)
    {
        player->crouched = false;
        player->maxs.z = MG_PLAYER_HEIGHT;
        player->eye_pos.z = MG_PLAYER_HEIGHT - MG_PLAYER_EYE_OFFSET;
    }
}

void _mg_player_slidemove(mg_player_t *player, float delta_time)
{
    uint16_t current_iter = 0;
    uint16_t max_iter = 10;
    gs_vec3 start;
    gs_vec3 end;
    bsp_trace_t *trace = &(bsp_trace_t){.map = player->map};
    float32_t prev_frac;
    gs_vec3 prev_normal;

    while (delta_time > 0)
    {
        // Sanity check for infinite loops,
        // shouldn't really happen.
        if (current_iter >= max_iter)
        {
            break;
        }
        current_iter++;

        start = player->transform.position;
        end = gs_vec3_add(start, gs_vec3_scale(player->velocity, delta_time));

        bsp_trace_box(
            trace,
            start,
            end,
            player->mins,
            player->maxs);

        if (trace->start_solid)
        {
            // Stuck in a solid, try to get out
            if (trace->all_solid)
            {
                _mg_player_unstuck(player);
            }
            else
            {
                gs_println("WARN: player start solid but not stuck: [%f, %f, %f] -> [%f, %f, %f]",
                           player->transform.position.x,
                           player->transform.position.y,
                           player->transform.position.z,
                           trace->end.x,
                           trace->end.y,
                           trace->end.z);
                player->transform.position = trace->end;
                player->velocity = gs_v3(0, 0, 0);
            }
            break;
        }

        if (trace->fraction > 0)
        {
            // Move as far as we can
            player->transform.position = trace->end;
        }

        if (trace->fraction == 1.0f)
        {
            // Moved all the way
            break;
        }

        if (current_iter > 1)
        {
            if (prev_frac == trace->fraction && gs_vec3_dot(prev_normal, trace->normal) > 1.0f - GS_EPSILON)
            {
                // Not going anywhere
                break;
            }
        }

        prev_frac = trace->fraction;
        prev_normal = trace->normal;

        delta_time -= delta_time * trace->fraction;

        // Slide along the plane, modify velocity for next loop
        player->velocity = mg_clip_velocity(player->velocity, trace->normal, 1.001f);
    }
}

void _mg_player_unstuck(mg_player_t *player)
{
    player->velocity = gs_v3(0, 0, 0);

    // Directions to attempt to teleport to,
    // in listed order.
    gs_vec3 directions[6] = {
        MG_AXIS_UP,
        MG_AXIS_FORWARD,
        MG_AXIS_RIGHT,
        MG_AXIS_BACKWARD,
        MG_AXIS_LEFT,
        MG_AXIS_DOWN,
    };

    float distance;
    float increment = 64.0f;
    float max_distance = increment * 10;
    uint32_t dir;
    gs_vec3 start;
    gs_vec3 end;
    bsp_trace_t *trace = &(bsp_trace_t){.map = player->map};

    while (true)
    {
        // Sweep player aabb by 1 unit
        start = gs_vec3_add(player->transform.position, gs_vec3_scale(directions[dir], distance));
        end = gs_vec3_add(start, directions[dir]);
        bsp_trace_box(trace, start, end, player->mins, player->maxs);

        if (trace->fraction > 0.0f && !trace->all_solid)
        {
            // Trace ends in a valid position.
            // Trace back towards start so we move
            // the minimum distance to get unstuck.
            gs_vec3 valid_pos = trace->end;
            bsp_trace_box(trace, trace->end, player->transform.position, player->mins, player->maxs);
            if (trace->fraction < 1.0f && !trace->all_solid)
            {
                valid_pos = trace->end;
            }

            player->transform.position = valid_pos;
            gs_println("WARN: player stuck in solid at [%f, %f, %f], freeing to [%f, %f, %f].",
                       player->transform.position.x,
                       player->transform.position.y,
                       player->transform.position.z,
                       valid_pos.x,
                       valid_pos.y,
                       valid_pos.z);
            break;
        }

        // Increment distance in current direction,
        // or swap to the next direction.
        distance += increment;
        if (distance > max_distance)
        {
            distance = 0;
            dir++;
            if (dir >= 6)
            {
                gs_println("WARN: player stuck in solid at [%f, %f, %f], could not unstuck.",
                           player->transform.position.x,
                           player->transform.position.y,
                           player->transform.position.z);
                break;
            }
        }
    }
}