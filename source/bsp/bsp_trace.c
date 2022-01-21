/*================================================================
	* bsp/bsp_trace.c
	*
	* Copyright (c) 2021 Lauri Räsänen
	* ================================

	BSP brush tracing for collisions.
	Based on post by Nathan Ostgard:
	http://www.devmaster.net/articles/quake3collision/
=================================================================*/

#include "bsp_trace.h"

void bsp_trace_ray(bsp_trace_t *trace, gs_vec3 start, gs_vec3 end, int32_t content_mask)
{
	trace->type   = RAY;
	trace->radius = 0;
	_bsp_trace(trace, start, end, content_mask);
}

void bsp_trace_sphere(bsp_trace_t *trace, gs_vec3 start, gs_vec3 end, float32_t radius, int32_t content_mask)
{
	trace->type   = SPHERE;
	trace->radius = radius;
	_bsp_trace(trace, start, end, content_mask);
}

void bsp_trace_box(bsp_trace_t *trace, gs_vec3 start, gs_vec3 end, gs_vec3 mins, gs_vec3 maxs, int32_t content_mask)
{
	if (gs_vec3_len2(mins) < BSP_TRACE_EPSILON && gs_vec3_len2(maxs) < BSP_TRACE_EPSILON)
	{
		// Treat as a ray
		bsp_trace_ray(trace, start, end, content_mask);
		return;
	}

	trace->type    = BOX;
	trace->radius  = 0;
	trace->mins    = mins;
	trace->maxs    = maxs;
	trace->extents = gs_v3(
		gs_max(-trace->mins.x, trace->maxs.x),
		gs_max(-trace->mins.y, trace->maxs.y),
		gs_max(-trace->mins.z, trace->maxs.z));

	_bsp_trace(trace, start, end, content_mask);
}

void _bsp_trace(bsp_trace_t *trace, gs_vec3 start, gs_vec3 end, int32_t content_mask)
{
	gs_assert(trace->map != NULL);

	trace->start_solid = false;
	trace->all_solid   = false;
	trace->fraction	   = 1.0f;

	// Walk through the BSP tree
	_bsp_trace_check_node(trace, 0, 0.0f, 1.0f, start, end, content_mask);

	if (trace->fraction == 1.0f)
	{
		// Nothing blocked the trace
		trace->end = end;
	}
	else
	{
		// Collided with something
		//           start + fraction * (end - start);
		trace->end = gs_vec3_add(start, gs_vec3_scale(gs_vec3_sub(end, start), trace->fraction));
	}
}

void _bsp_trace_check_node(bsp_trace_t *trace, int32_t node_index, float32_t start_fraction, float32_t end_fraction, gs_vec3 start, gs_vec3 end, int32_t content_mask)
{
	if (node_index < 0)
	{
		// This is a leaf
		bsp_leaf_lump_t leaf = trace->map->leaves.data[-(node_index + 1)];
		int32_t brush_index;
		bsp_brush_lump_t brush;
		for (size_t i = 0; i < leaf.num_leaf_brushes; i++)
		{
			brush_index = trace->map->leaf_brushes.data[leaf.first_leaf_brush + i].brush;
			brush	    = trace->map->brushes.data[brush_index];
			if (brush.num_brush_sides > 0 && (trace->map->textures.data[brush.texture].contents & content_mask) != 0)
			{
				_bsp_trace_check_brush(trace, brush, start, end);
			}
		}

		// Don't have to do anything else for leaves
		return;
	}

	// This is a node
	bsp_node_lump_t node   = trace->map->nodes.data[node_index];
	bsp_plane_lump_t plane = trace->map->planes.data[node.plane];

	float32_t offset;
	float32_t start_distance = gs_vec3_dot(start, plane.normal) - plane.dist;
	float32_t end_distance	 = gs_vec3_dot(end, plane.normal) - plane.dist;

	if (trace->type == SPHERE)
	{
		offset = trace->radius;
	}
	else if (trace->type == BOX)
	{
		// Dot product but we want the absolute values
		offset = fabsf(trace->extents.x * plane.normal.x) +
			 fabsf(trace->extents.y * plane.normal.y) +
			 fabsf(trace->extents.z * plane.normal.z);
	}

	if (start_distance >= offset && end_distance >= offset)
	{
		// Both points are in front of the plane,
		// check the front child.
		_bsp_trace_check_node(trace, node.children[0], start_fraction, end_fraction, start, end, content_mask);
	}
	else if (start_distance < -offset && end_distance < -offset)
	{
		// Both points are behind the plane,
		// check back child.
		_bsp_trace_check_node(trace, node.children[1], start_fraction, end_fraction, start, end, content_mask);
	}
	else
	{
		// The line crosses through the splitting plane.
		int32_t side;
		float32_t fraction1;
		float32_t fraction2;
		float32_t middle_fraction;
		gs_vec3 middle;

		// STEP 1: Split the segment into two.
		if (start_distance < end_distance)
		{
			side			   = 1; // back
			float32_t inverse_distance = 1.0f / (start_distance - end_distance);
			fraction1		   = (start_distance - offset + BSP_TRACE_EPSILON) * inverse_distance;
			fraction2		   = (start_distance + offset + BSP_TRACE_EPSILON) * inverse_distance;
		}
		else if (end_distance < start_distance)
		{
			side			   = 0; // front
			float32_t inverse_distance = 1.0f / (start_distance - end_distance);
			fraction1		   = (start_distance + offset + BSP_TRACE_EPSILON) * inverse_distance;
			fraction2		   = (start_distance - offset - BSP_TRACE_EPSILON) * inverse_distance;
		}
		else
		{
			side	  = 0; // front
			fraction1 = 1.0f;
			fraction2 = 0.0f;
		}

		// STEP 2: Make sure the numbers are valid.
		fraction1 = fminf(1.0f, fmaxf(0.0f, fraction1));
		fraction2 = fminf(1.0f, fmaxf(0.0f, fraction2));

		// STEP 3: Calculate the middle point for the first side
		middle_fraction = start_fraction + (end_fraction - start_fraction) * fraction1;
		middle		= gs_vec3_add(start, gs_vec3_scale(gs_vec3_sub(end, start), fraction1));

		// STEP 4: Check the first side
		_bsp_trace_check_node(trace, node.children[side], start_fraction, middle_fraction, start, middle, content_mask);

		// STEP 5: Calculate the middle point for the second side
		middle_fraction = start_fraction + (end_fraction - start_fraction) * fraction2;
		middle		= gs_vec3_add(start, gs_vec3_scale(gs_vec3_sub(end, start), fraction2));

		// STEP 6: Check the second side
		_bsp_trace_check_node(trace, node.children[1 - side], middle_fraction, end_fraction, middle, end, content_mask);
	}
}

void _bsp_trace_check_brush(bsp_trace_t *trace, bsp_brush_lump_t brush, gs_vec3 start, gs_vec3 end)
{
	float32_t start_fraction = -1.0f;
	float end_fraction	 = 1.0f;
	bool32_t starts_out	 = false;
	bool32_t ends_out	 = false;

	bsp_brush_side_lump_t brush_side;
	bsp_brush_side_lump_t clip_brush_side;
	bsp_plane_lump_t plane;
	bsp_plane_lump_t clip_plane;
	gs_vec3 offset = gs_v3(0, 0, 0);
	float32_t start_distance;
	float32_t end_distance;
	float32_t fraction;

	for (size_t i = 0; i < brush.num_brush_sides; i++)
	{
		brush_side = trace->map->brush_sides.data[brush.first_brush_side + i];
		plane	   = trace->map->planes.data[brush_side.plane];
		offset	   = gs_v3(0, 0, 0);

		if (trace->type == BOX)
		{
			for (size_t j = 0; j < 3; j++)
			{
				if (plane.normal.xyz[j] < 0)
				{
					offset.xyz[j] = trace->maxs.xyz[j];
				}
				else
				{
					offset.xyz[j] = trace->mins.xyz[j];
				}
			}
		}

		// NOTE: offset should be zero if not BOX,
		// and radius should be zero if not SPHERE.
		start_distance = gs_vec3_dot(gs_vec3_add(start, offset), plane.normal) - (plane.dist + trace->radius);
		end_distance   = gs_vec3_dot(gs_vec3_add(end, offset), plane.normal) - (plane.dist + trace->radius);

		// Don't reset these to false,
		// we are outside the brush if in front of any plane.
		if (start_distance > 0)
			starts_out = true;
		if (end_distance > 0)
			ends_out = true;

		// Make sure the trace isn't completely on one side of the plane.
		// Make sure end_distance can't get too close to the plane.
		if (start_distance > 0 && (end_distance >= BSP_TRACE_EPSILON || end_distance >= start_distance))
		{
			// Both are in front of the plane, outside of the brush
			return;
		}
		if (start_distance <= 0 && end_distance <= 0)
		{
			// Both are behind this plane, check the next one
			continue;
		}

		if (start_distance > end_distance)
		{
			// The line is entering the plane
			fraction = (start_distance - BSP_TRACE_EPSILON) / (start_distance - end_distance);
			if (fraction < 0)
			{
				fraction = 0;
			}
			if (fraction > start_fraction)
			{
				start_fraction	= fraction;
				clip_plane	= plane;
				clip_brush_side = brush_side;
			}
		}
		else
		{
			// The line is leaving the plane
			fraction = (start_distance + BSP_TRACE_EPSILON) / (start_distance - end_distance);
			if (fraction > 1.0f)
			{
				fraction = 1.0f;
			}
			if (fraction < end_fraction)
			{
				end_fraction = fraction;
			}
		}
	}

	// TODO: What if we start inside but end outside a previous brush,
	// and start outside but end inside the current one?
	// start_solid will be true but all_solid will be false despite ending inside.
	if (!starts_out)
	{
		trace->start_solid = true;
		if (!ends_out)
		{
			trace->all_solid = true;
			trace->fraction	 = 0;
			trace->contents	 = trace->map->textures.data[brush.texture].contents;
		}
		return;
	}

	if (start_fraction < end_fraction)
	{
		if (start_fraction > -1.0f && start_fraction < trace->fraction)
		{
			trace->fraction	     = fmaxf(0.0f, start_fraction);
			trace->normal	     = clip_plane.normal;
			trace->contents	     = trace->map->textures.data[brush.texture].contents;
			trace->surface_flags = trace->map->textures.data[clip_brush_side.texture].flags;
		}
	}
}