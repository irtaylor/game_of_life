
#if 0
#include "cross_platform.h"

internal void
game_update_and_render(game_graphics_buffer *buffer)
{
    local_persist u8 blue_offset  = 0;
    local_persist u8 green_offset = 0;
    {
        u32 *pixel = (u32 *)buffer->memory;
        for(int y = 0;
            y < buffer->.height;
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
    blue_offset += 1;
    green_offset += 1;
}
#endif
