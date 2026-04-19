
#include "map_renderer.h"

void populate_map_texture(Map *map, unsigned char *tiles_sprite, int tiles_sprite_w)
{
	map->wall_texture.texture_data = exctract_sprite_sheet_sample(tiles_sprite, {16 * 1, 16 * 2}, {16 * 4, 16 * 5}, tiles_sprite_w);
	map->wall_texture.texture_width = 16;
	map->wall_texture.texture_height = 16;

	map->left_wall_texture.texture_data = exctract_sprite_sheet_sample(tiles_sprite, {16 * 5, 16 * 6}, {16 * 0, 16 * 1}, tiles_sprite_w);
	map->left_wall_texture.texture_width = 16;
	map->left_wall_texture.texture_height = 16;

	map->right_wall_texture.texture_data = exctract_sprite_sheet_sample(tiles_sprite, {16 * 0, 16 * 1}, {16 * 0, 16 * 1}, tiles_sprite_w);
	map->right_wall_texture.texture_width = 16;
	map->right_wall_texture.texture_height = 16;

	map->empty_texture.texture_data = exctract_sprite_sheet_sample(tiles_sprite, {16 * 1, 16 * 2}, {16 * 1, 16 * 2}, tiles_sprite_w);
	map->empty_texture.texture_width = 16;
	map->empty_texture.texture_height = 16;
	
	map->top_left_wall_texture.texture_data = exctract_sprite_sheet_sample(tiles_sprite, {16 * 0, 16 * 1}, {16 * 5, 16 * 6}, tiles_sprite_w);
	map->top_left_wall_texture.texture_width = 16;
	map->top_left_wall_texture.texture_height = 16;

	map->bottom_left_wall_texture.texture_data = exctract_sprite_sheet_sample(tiles_sprite, {16 * 0, 16 * 1}, {16 * 4, 16 * 5}, tiles_sprite_w);
	map->bottom_left_wall_texture.texture_width = 16;
	map->bottom_left_wall_texture.texture_height = 16;

	map->check_point_texture.texture_data = exctract_sprite_sheet_sample(tiles_sprite, {16 * 0, 16 * 1}, {16 * 9, 16 * 10}, tiles_sprite_w);
	map->check_point_texture.texture_width = 16;
	map->check_point_texture.texture_height = 16;

	map->exit_texture.texture_data = exctract_sprite_sheet_sample(tiles_sprite, {16 * 9, 16 * 10}, {16 * 3, 16 * 4}, tiles_sprite_w - 1);
	map->exit_texture.texture_width = 16;
	map->exit_texture.texture_height = 16;

	map->bottom_wall_texture.texture_data = exctract_sprite_sheet_sample(tiles_sprite, {16 * 8, 16 * 9}, {16 * 7, 16 * 8}, tiles_sprite_w);
	map->bottom_wall_texture.texture_width = 16;
	map->bottom_wall_texture.texture_height = 16;
}

void render_scene(Map *map, window_canvas_t quad, float w, float h, vec2_t offset)
{
	for (int y = 0; y < map->height + 0 ; ++y)
	{
		for (int x = 0; x < map->width + 0 ; ++x)
		{
			switch (map->tile_types[y][x])
			{
				case TILE_WALL_INDESTRUCTIBLE:
					quad.texturee.texture_data   = map->wall_texture.texture_data;
					quad.texturee.texture_width  = map->wall_texture.texture_width;
					quad.texturee.texture_height = map->wall_texture.texture_height;
					break;
				case TILE_WALL_LEFT_EDGE:
					quad.texturee.texture_data   = map->left_wall_texture.texture_data;
					quad.texturee.texture_width  = map->left_wall_texture.texture_width;
					quad.texturee.texture_height = map->left_wall_texture.texture_height;
					break;
				case TILE_WALL_RIGHT_EDGE:
					quad.texturee.texture_data   = map->right_wall_texture.texture_data;
					quad.texturee.texture_width  = map->right_wall_texture.texture_width;
					quad.texturee.texture_height = map->right_wall_texture.texture_height;
					break;
				case TILE_SPIKE:
					quad.texturee.texture_data   = map->spike_texture.texture_data;
					quad.texturee.texture_width  = map->spike_texture.texture_width;
					quad.texturee.texture_height = map->spike_texture.texture_height;
					break;
				case TILE_WALL_TOP_LEFT_CORNER:
					quad.texturee.texture_data   = map->top_left_wall_texture.texture_data;
					quad.texturee.texture_width  = map->top_left_wall_texture.texture_width;
					quad.texturee.texture_height = map->top_left_wall_texture.texture_height;
					break;
				case TILE_WALL_BOTTOM_LEFT_CORNER:
					quad.texturee.texture_data   = map->bottom_left_wall_texture.texture_data;
					quad.texturee.texture_width  = map->bottom_left_wall_texture.texture_width;
					quad.texturee.texture_height = map->bottom_left_wall_texture.texture_height;
					break;
				case TILE_CHECKPOINT:
					quad.texturee.texture_data   = map->check_point_texture.texture_data;
					quad.texturee.texture_width  = map->check_point_texture.texture_width;
					quad.texturee.texture_height = map->check_point_texture.texture_height;
					break;
				case TILE_EXIT:
					quad.texturee.texture_data   = map->exit_texture.texture_data;
					quad.texturee.texture_width  = map->exit_texture.texture_width;
					quad.texturee.texture_height = map->exit_texture.texture_height;
					break;
				case TILE_WALL_BOTTOM_EDGE:
					quad.texturee.texture_data   = map->bottom_wall_texture.texture_data;
					quad.texturee.texture_width  = map->bottom_wall_texture.texture_width;
					quad.texturee.texture_height = map->bottom_wall_texture.texture_height;
					break;
				default:
					break;
			}
			{
				quad.position.x = (w/((float)SCREEN_WIDTH/2)) * x + offset.x;
				quad.position.y = (h/((float)SCREEN_HEIGHT/2)) * (map->height - y) + offset.y;
				if (map->tile_types[y][x] != TILE_EMPTY && map->tile_types[y][x] != TILE_PLAYER_START)
				{
					if (map->tile_types[y][x] != TILE_EMPTY && map->tile_types[y][x] != TILE_CHECKPOINT)
						glStencilMask(0xFF);
					else
						glStencilMask(0x0);
					setup_quad_screen(quad);
					render_quad_screen(quad, 0, 0);
				}
			}
		}
	}
}