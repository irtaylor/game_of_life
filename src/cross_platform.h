#ifndef CROSS_PLATFORM_H

#include <stdint.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef float  f32;
typedef double f64;

typedef int32_t bool32;

#define local_persist static
#define internal static
#define global_variable static

#define Assert(expression) if(!(expression)) {*(int *)0 = 0;}

#define Kilobytes(value) ((value) * 1024LL)
#define Megabytes(value) (Kilobytes(value) * 1024LL)
#define Gigabytes(value) (Megabytes(value) * 1024LL)
#define Terabytes(value) (Gigabytes(value) * 1024LL)

#define Array_Count(array) (sizeof(array) / sizeof((array)[0]))

struct game_graphics_buffer
{
    // NOTE(ian): Pixels are always 32-bits wide, Memory Order BB GG RR XX
    // BITMAPINFO defines dimensions and color info for a DIB (device-independet buffer)
    void *memory;
    int width;
    int height;
    int bytes_per_row;
    int bytes_per_pixel;
};

struct game_memory
{
    bool32 is_initialized;
    size_t storage_size;
    size_t used;
    void *storage_memory;
};

struct game_input
{
    f32 animation_speed_factor;
    u32 scaling_factor;
    int mouse_x;
    int mouse_y;
    union
    {
        bool32 button_states[8];
        struct
        {
            bool32 mouse_left;
            bool32 mouse_right;

            bool32 up;
            bool32 down;
            bool32 left;
            bool32 right;

            bool32 run_simulation;
            bool32 reset;
        };
    };
};

struct color
{
    f32 r;
    f32 g;
    f32 b;
};

internal void draw_rectangle(game_graphics_buffer *buffer,
                             int min_x, int min_y, int max_x, int max_y,
                             color rect_color)
{
    // TODO(ian): mathematically round these values instead of truncating them...
    u32 red   = u32(rect_color.r * 255.0f);
    u32 green = u32(rect_color.g * 255.0f);
    u32 blue  = u32(rect_color.b * 255.0f);

    u32 pixel_color = ((red << 16) | (green << 8) | blue);

    if(min_x < 0)
    {
        min_x = 0;
    }
    if(min_y < 0)
    {
        min_y = 0;
    }
    if(max_x >= buffer->width)
    {
        max_x = buffer->width;
    }
    if(max_y >= buffer->height)
    {
        max_y = buffer->height;
    }

    u8 *row = ((u8 *)buffer->memory +
                     min_y*buffer->bytes_per_row +
                     min_x*buffer->bytes_per_pixel);
    for(int y = min_y;
        y < max_y;
        y += 1)
    {
        u32 *pixel = (u32 *)row;
        for(int x = min_x;
            x < max_x;
            x += 1)
        {
            *pixel++ = pixel_color;
        }
        row += buffer->bytes_per_row;
    }
}

// TODO(ian): user should be able to control the rows, columns, and
// grid scaling...
#define GRID_ROWS 36
#define GRID_COLUMNS 64

internal void
init_grid(bool32 *grid)
{
    for(int row = 0;
        row < GRID_ROWS;
        row += 1)
    {
        for(int col = 0;
            col < GRID_COLUMNS;
            col += 1)
        {
            grid[row * GRID_COLUMNS + col] = 0;
        }
    }
}


#define Push_Array(mem_block, count, type) (type *)_push_size(mem_block, (count)*sizeof(type))
void *
_push_size(game_memory *memory, u64 size)
{
    Assert((memory->used + size) <= memory->storage_size);
    void *result = (u8 *)memory->storage_memory + memory->used;
    memory->used += size;

    return(result);
}

internal void
game_update_and_render(game_graphics_buffer *buffer,
                       game_memory *memory,
                       game_input new_input,
                       game_input old_input)
{
#if 1
    // DRAW A WEIRD GRADIENT FOR DEBUG PURPOSES
    {
        local_persist u8 blue_offset  = 0;
        local_persist u8 green_offset = 0;
        {
            u32 *pixel = (u32 *)buffer->memory;
            for(int y = 0;
                y < buffer->height;
                y += 1)
            {
                for(int x = 0;
                    x < buffer->width;
                    x += 1)
                {
                    u8 blue = (u8)y + blue_offset;
                    u8 green = (u8)x + green_offset;
                    u8 red = (u8)(blue + green);

                    *pixel++ = ((red << 16) | (green << 8) | blue);
                }
            }
        }
    }
#endif

    local_persist bool32 *grid = 0;
    local_persist bool32 *temp_grid = 0;
    if(!memory->is_initialized)
    {
        grid = (bool32 *)Push_Array(memory, GRID_ROWS * GRID_COLUMNS, bool32);
        temp_grid = (bool32 *)Push_Array(memory, GRID_ROWS * GRID_COLUMNS, bool32);
        init_grid(grid);
        memory->is_initialized = true;
    }

    int tile_top_x = 0;
    int tile_top_y = 0;
    int tile_side_in_pixels = 15;
    int tile_side_in_meters = 1;
    int tile_pad = 1;

    color tile_border_color = {0.5f, 0.5f, 0.5f};
    color tile_off_color = {1.0f, 1.0f, 1.0f};
    color tile_on_color = {0.0f, 0.0f, 0.0f};
    color grid_border_color = {0.25f, 0.25f, 0.25f};

    if(!new_input.run_simulation)
    {
        if(new_input.mouse_left)
        {
            local_persist bool32 toggle_on;
            local_persist int prev_tile_x;
            local_persist int prev_tile_y;

            int cur_tile_x = (new_input.mouse_x / tile_side_in_pixels) / new_input.scaling_factor;
            int cur_tile_y = (new_input.mouse_y / tile_side_in_pixels) / new_input.scaling_factor;

            if((0 <= cur_tile_x && cur_tile_x < GRID_COLUMNS) &&
               (0 <= cur_tile_y && cur_tile_y < GRID_ROWS))
            {
                // NOTE(ian): Here, we check for a simple mouse-click,
                // as in a situation where the user is just trying to toggle
                // tiles.
                if(new_input.mouse_left != old_input.mouse_left)
                {
                    toggle_on = !grid[cur_tile_y * GRID_COLUMNS + cur_tile_x];
                    grid[cur_tile_y * GRID_COLUMNS + cur_tile_x] = !grid[cur_tile_y * GRID_COLUMNS + cur_tile_x];
                }
                // NOTE(ian): Here, we held the mouse mouse button down and dragged.
                // We check that the current tile is different from the previous one
                // so that we can check a situation where the user is trying to
                // "paint" the tiles.
                //
                // In that case, we paint tiles based on what we previously clicked.
                // E.g., if the user previously clicked an off-tile, then they will
                // paint with on-tiles, and if they previously clicked an on-tile,
                // they will paint with off-tiles.
                else
                {
                    if (cur_tile_x != prev_tile_x ||
                         cur_tile_y != prev_tile_y)
                    {
                        grid[cur_tile_y * GRID_COLUMNS + cur_tile_x] = toggle_on;
                    }
                }

            }
            prev_tile_x = cur_tile_x;
            prev_tile_y = cur_tile_y;
        }
    }
    else
    {
        init_grid(temp_grid);

        for(int row = 0;
            row < GRID_ROWS;
            row += 1)
        {
            for(int col = 0;
                col < GRID_COLUMNS;
                col += 1)
            {
                // TODO(ian): Simulate the game for ALL squares in the grid.
                // Currently, I'm not handling edges or offscreen...
                if(row != 0 && row != GRID_ROWS - 1 &&
                   col != 0 && col != GRID_COLUMNS - 1)
                {
                    bool32 live_neighbors_count = 0;
                    live_neighbors_count += grid[(row - 1) * GRID_COLUMNS + (col - 1)];
                    live_neighbors_count += grid[(row - 1) * GRID_COLUMNS + (col)];
                    live_neighbors_count += grid[(row - 1) * GRID_COLUMNS + (col + 1)];

                    live_neighbors_count += grid[row * GRID_COLUMNS + (col - 1)];
                    live_neighbors_count += grid[row * GRID_COLUMNS + (col + 1)];

                    live_neighbors_count += grid[(row + 1) * GRID_COLUMNS + (col - 1)];
                    live_neighbors_count += grid[(row + 1) * GRID_COLUMNS + (col)];
                    live_neighbors_count += grid[(row + 1) * GRID_COLUMNS + (col + 1)];

                    if(grid[row * GRID_COLUMNS + col])
                    {
                        if(live_neighbors_count == 2 || live_neighbors_count == 3)
                        {
                            temp_grid[row * GRID_COLUMNS + col] = 1;
                        }
                    }
                    else
                    {
                        if(live_neighbors_count == 3)
                        {
                            temp_grid[row * GRID_COLUMNS + col] = 1;
                        }
                    }
                }

            }
        }

        // TODO(ian): better way to copy an array?
        for(int row = 0;
            row < GRID_ROWS;
            row += 1)
        {
            for(int col = 0;
                col < GRID_COLUMNS;
                col += 1)
            {
                grid[row * GRID_COLUMNS + col] = temp_grid[row * GRID_COLUMNS + col];
            }
        }
    }

    // DRAW_GRID
    {
        for(int row = 0;
            row < GRID_ROWS;
            row += 1)
        {
            for(int col = 0;
                col < GRID_COLUMNS;
                col += 1)
            {
                // draw outer rect

                draw_rectangle(buffer,
                               tile_top_x, tile_top_y,
                               tile_top_x + tile_side_in_pixels,
                               tile_top_y + tile_side_in_pixels,
                               tile_border_color);
                // draw inner rect
                if(grid[row * GRID_COLUMNS + col])
                {
                    draw_rectangle(buffer,
                                   tile_top_x + tile_pad, tile_top_y + tile_pad,
                                   tile_top_x + tile_side_in_pixels - tile_pad,
                                   tile_top_y + tile_side_in_pixels - tile_pad,
                                   tile_on_color);
                }
                else if(row == 0 || row == (GRID_ROWS - 1) ||
                        col == 0 || col == (GRID_COLUMNS - 1))
                {
                    draw_rectangle(buffer,
                                   tile_top_x + tile_pad, tile_top_y + tile_pad,
                                   tile_top_x + tile_side_in_pixels - tile_pad,
                                   tile_top_y + tile_side_in_pixels - tile_pad,
                                   grid_border_color);
                }
                else
                {
                    draw_rectangle(buffer,
                                   tile_top_x + tile_pad, tile_top_y + tile_pad,
                                   tile_top_x + tile_side_in_pixels - tile_pad,
                                   tile_top_y + tile_side_in_pixels - tile_pad,
                                   tile_off_color);
                }
                tile_top_x += tile_side_in_pixels;
            }
            tile_top_x = 0;
            tile_top_y += tile_side_in_pixels;
        }
    }
#if 0
    // DRAW MOUSE:
    // Construct a square centered on the cursor position.
    {
        color mouse_left_color = {1.0f, 0.0f, 0.0f};
        color mouse_right_color = {0.0f, 0.0f, 1.0f};

        int mouse_tile_side_in_pixels = 50;
        int mouse_tile_side_in_meters = 1;
        f32 meters_to_pixels = (f32)mouse_tile_side_in_pixels / (f32)mouse_tile_side_in_meters;

        int mouse_min_x = (new_input.mouse_x - (int)((f32)mouse_tile_side_in_meters * meters_to_pixels / 2)) / new_input.scaling_factor;
        int mouse_min_y = (new_input.mouse_y - (int)((f32)mouse_tile_side_in_meters * meters_to_pixels / 2)) / new_input.scaling_factor;
        int mouse_max_x = (new_input.mouse_x + (int)((f32)mouse_tile_side_in_meters * meters_to_pixels / 2)) / new_input.scaling_factor;
        int mouse_max_y = (new_input.mouse_y + (int)((f32)mouse_tile_side_in_meters * meters_to_pixels / 2)) / new_input.scaling_factor;
        if(true)
        {
            draw_rectangle(buffer,
                           mouse_min_x, mouse_min_y,
                           mouse_max_x, mouse_max_y,
                           mouse_left_color);
        }
        if(new_input.mouse_left)
        {
            color click_color_1 = {0.0f, 1.0f, 0.0f};
            draw_rectangle(buffer,
                           mouse_min_x, mouse_min_y,
                           mouse_max_x, mouse_max_y,
                           click_color_1);

            color click_color_2 = {0.0f, 0.0f, 1.0f};
            draw_rectangle(buffer,
                           new_input.mouse_x, new_input.mouse_y,
                           new_input.mouse_x + mouse_tile_side_in_pixels,
                           new_input.mouse_y + mouse_tile_side_in_pixels,
                           click_color_2);
        }
        if(new_input.mouse_right)
        {

            draw_rectangle(buffer,
                           mouse_min_x, mouse_min_y,
                           mouse_max_x, mouse_max_y,
                           mouse_right_color);
        }
    }
#endif

#if 0
    local_persist int player_min_x = 0;
    local_persist int player_min_y = 0;

    f32 delta_player_x = 0.0f;
    f32 delta_player_y = 0.0f;
    f32 player_speed = 1.0f;

    if(new_input.up)
    {
        delta_player_y = -1.0f;
    }
    if(new_input.down)
    {
        delta_player_y = 1.0f;
    }
    if(new_input.right)
    {
        delta_player_x = 1.0f;
    }
    if(new_input.left)
    {
        delta_player_x = -1.0f;
    }

    int player_width = 40;
    int player_height = 40;

    player_min_x += (int)(delta_player_x * player_speed);
    player_min_y += (int)(delta_player_y * player_speed);
    int player_max_x = player_min_x + player_width;
    int player_max_y = player_min_y + player_height;

    // DRAW PLAYER
    {
        color player_color = {0.0f, 1.0f, 0.0f};
        draw_rectangle(buffer,
                       player_min_x, player_min_y,
                       player_max_x, player_max_y,
                       player_color);
    }
#endif
    // blue_offset += 1;
    // green_offset += 1;
}

#define CROSS_PLATFORM_H
#endif
