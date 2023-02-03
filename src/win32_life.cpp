#include "cross_platform.h"
// #include "game_of_life.h"

#include <windows.h>
#include <stdio.h>

struct win32_window_dimension
{
    int height;
    int width;
};

struct win32_graphics_buffer
{
    // NOTE(ian): Pixels are always 32-bits wide, Memory Order BB GG RR XX
    // BITMAPINFO defines dimensions and color info for a DIB (device-independet buffer)
    BITMAPINFO win32_dib_info;
    void *memory;
    int width;
    int height;
    int bytes_per_row;
    int bytes_per_pixel;
};


global_variable bool global_running;
global_variable bool global_pause;

// TODO(ian): i would love this to NOT be a global, but presently it
// seems like it must be so, because Windows can call us in the callback...
global_variable win32_graphics_buffer global_win32_graphics_buffer;
global_variable WINDOWPLACEMENT GlobalWindowPosition = {sizeof(GlobalWindowPosition)};
global_variable bool32 global_is_fullscreen = false;


inline LARGE_INTEGER
win32_get_wall_clock(void)
{
    LARGE_INTEGER result;
    QueryPerformanceCounter(&result);
    return(result);
}

inline f32
win32_get_seconds_elapsed(s64 perf_counter_frequency,
                          LARGE_INTEGER start, LARGE_INTEGER end)
{
    f32 result = ((f32)(end.QuadPart - start.QuadPart) /
                       (f32)perf_counter_frequency);
    return(result);
}


internal win32_window_dimension
win32_get_window_dimension(HWND window)
{
    win32_window_dimension result;

    RECT client_rect;
    GetClientRect(window, &client_rect);
    result.width = client_rect.right - client_rect.left;
    result.height = client_rect.bottom - client_rect.top;

    return result;
}

internal void
toggle_fullscreen(HWND Window)
{
    // NOTE(casey): This follows Raymond Chen's prescription
    // for fullscreen toggling, see:
    // http://blogs.msdn.com/b/oldnewthing/archive/2010/04/12/9994016.aspx

    DWORD Style = GetWindowLong(Window, GWL_STYLE);
    if(Style & WS_OVERLAPPEDWINDOW)
    {
        MONITORINFO MonitorInfo = {sizeof(MonitorInfo)};
        if(GetWindowPlacement(Window, &GlobalWindowPosition) &&
           GetMonitorInfo(MonitorFromWindow(Window, MONITOR_DEFAULTTOPRIMARY), &MonitorInfo))
        {
            SetWindowLong(Window, GWL_STYLE, Style & ~WS_OVERLAPPEDWINDOW);
            SetWindowPos(Window, HWND_TOP,
                         MonitorInfo.rcMonitor.left, MonitorInfo.rcMonitor.top,
                         MonitorInfo.rcMonitor.right - MonitorInfo.rcMonitor.left,
                         MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top,
                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    }
    else
    {
        SetWindowLong(Window, GWL_STYLE, Style | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(Window, &GlobalWindowPosition);
        SetWindowPos(Window, 0, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                     SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}

internal bool32
update_input_state(bool32 current_input, bool32 is_down)
{
    bool32 result = current_input;
    if(current_input != is_down)
    {
        result = is_down;
    }
    return result;
}

internal void
win32_process_pending_messages(game_input *new_input)
{
    MSG Message;
    while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
    {
        switch(Message.message)
        {
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_KEYUP:
            {
                u32 vk_code = (u32)Message.wParam;

                // NOTE(casey): Since we are comparing WasDown to IsDown,
                // we MUST use == and != to convert these bit tests to actual
                // 0 or 1 values.

                // NOTE(ian): See docs for WM KEYUP/KEYDOWN messages.
                // These docs explain how to interpret the lParam as
                // a bit field, and so then we check the desired bit
                // to determine what's happening to the key in question.
                bool32 was_down = ((Message.lParam & (1 << 30)) != 0);
                bool32 is_down = ((Message.lParam & (1 << 31)) == 0);

                if(was_down != is_down)
                {
                    if(vk_code == 'W')
                    {
                        new_input->up = update_input_state(new_input->up, is_down);
                    }
                    if(vk_code == 'A')
                    {
                        new_input->left = update_input_state(new_input->left, is_down);
                    }
                    if(vk_code == 'S')
                    {
                        new_input->down = update_input_state(new_input->down, is_down);
                    }
                    if(vk_code == 'D')
                    {
                        new_input->right = update_input_state(new_input->right, is_down);
                    }
                    if(vk_code == 'R')
                    {
                        new_input->reset = update_input_state(new_input->reset, is_down);
                    }
                    if(vk_code == VK_OEM_PLUS)
                    {
                        new_input->animation_speed_factor *= 0.5f;
                    }
                    if(vk_code == VK_OEM_MINUS)
                    {
                        new_input->animation_speed_factor /= 0.5f;
                    }

                    if(vk_code == 'P')
                    {
                        if(is_down)
                        {
                            global_pause = !global_pause;
                        }
                    }
                    if(vk_code == VK_SPACE)
                    {
                        if(is_down)
                        {
                            new_input->run_simulation = !new_input->run_simulation;
                        }
                    }

                    if(is_down)
                    {
                        bool32 alt_key_was_down = (Message.lParam & (1 << 29));
                        if((vk_code == VK_F4) && alt_key_was_down)
                        {
                            global_running = false;
                        }
                        if((vk_code == VK_RETURN) && alt_key_was_down)
                        {
                            if(Message.hwnd)
                            {
                                toggle_fullscreen(Message.hwnd);
                                global_is_fullscreen = !global_is_fullscreen;
                                if(global_is_fullscreen)
                                {
                                    new_input->scaling_factor = 2;
                                }
                                else
                                {
                                    new_input->scaling_factor = 1;
                                }
                            }
                        }
                    }
                }

            } break;


            case WM_QUIT:
            {
                OutputDebugStringA("WM_QUIT\n");
                global_running = false;
            } break;
            default:
            {
                TranslateMessage(&Message);
                DispatchMessageA(&Message);
            } break;
        }
    }
}

internal void
win32_display_buffer_in_window(win32_graphics_buffer *buffer,
                               HDC device_context,
                               win32_window_dimension dimension)
{

    if((dimension.width >= buffer->width*2) &&
       (dimension.height >= buffer->height*2))
    {
        StretchDIBits(device_context,
                      0, 0, 2*buffer->width, 2*buffer->height,
                      0, 0, buffer->width, buffer->height,
                      buffer->memory,
                      &buffer->win32_dib_info,
                      DIB_RGB_COLORS, SRCCOPY);
    }
    else
    {
        int offset_x = 10;
        int offset_y = 10;

        PatBlt(device_context, 0, 0, dimension.width, offset_y, BLACKNESS);
        PatBlt(device_context, 0, offset_y + buffer->height, dimension.width, dimension.height, BLACKNESS);
        PatBlt(device_context, 0, 0, offset_x, dimension.height, BLACKNESS);
        PatBlt(device_context, offset_x + buffer->width, 0, dimension.width, dimension.height, BLACKNESS);

        // NOTE(casey): For prototyping purposes, we're going to always blit
        // 1-to-1 pixels to make sure we don't introduce artifacts with
        // stretching while we are learning to code the renderer!
        StretchDIBits(device_context,
                      offset_x, offset_y, buffer->width, buffer->height,
                      0, 0, buffer->width, buffer->height,
                      buffer->memory,
                      &buffer->win32_dib_info,
                      DIB_RGB_COLORS, SRCCOPY);

    }
}

internal LRESULT CALLBACK
win32_main_window_callback(HWND Window,
                           UINT Message,
                           WPARAM WParam,
                           LPARAM LParam)
{
    LRESULT Result = 0;
    switch(Message)
    {
        // case WM_SIZE:
        // {
        //     OutputDebugStringA("WM_SIZE\n");
        //     // TODO(ian): casey does something with this, correct?
        // } break;

        case WM_CLOSE:
        {
            OutputDebugStringA("WM_CLOSE\n");
            global_running = false;
        } break;

        case WM_ACTIVATEAPP:
        {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;

        case WM_DESTROY:
        {
            OutputDebugStringA("WM_DESTROY\n");
            global_running = false;
        } break;

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            Assert(!"Keyboard input came in through a non-dispatch message!");
        } break;

        case WM_PAINT:
        {
            PAINTSTRUCT Paint;
            HDC device_context = BeginPaint(Window, &Paint);
            win32_window_dimension dimension = win32_get_window_dimension(Window);
            win32_display_buffer_in_window(&global_win32_graphics_buffer, device_context, dimension);
            EndPaint(Window, &Paint);
        } break;

        default:
        {
            Result = DefWindowProc(Window, Message, WParam, LParam);
        } break;
    }
    return Result;
}


int CALLBACK
WinMain(HINSTANCE Instance,
        HINSTANCE PrevInstance,
        LPSTR CommandLine,
        int ShowCode)
{
    global_running = true;
    global_pause = false;

    LARGE_INTEGER perf_counter_frequency_result;
    QueryPerformanceFrequency(&perf_counter_frequency_result);
    s64 performance_counter_frequency = perf_counter_frequency_result.QuadPart;

    WNDCLASSA window_class = {};
    window_class.style = CS_HREDRAW|CS_VREDRAW;
    window_class.lpfnWndProc = win32_main_window_callback;
    window_class.hInstance = Instance;
    window_class.hCursor = LoadCursor(0, IDC_ARROW);

    window_class.lpszClassName = "Game_Of_Life_Window_Class";

    if(RegisterClassA(&window_class))
    {
        HWND Window =
            CreateWindowExA(
                0, // WS_EX_TOPMOST|WS_EX_LAYERED,
                window_class.lpszClassName,
                "Game Of Life",
                WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                0,
                0,
                Instance,
                0);
        if(Window)
        {
            // CONFIGURING THE GLOBAL GRAPHICS BUFFER
            {
                int width = 960;
                int height = 540;

                global_win32_graphics_buffer = {};
                global_win32_graphics_buffer.width = width;
                global_win32_graphics_buffer.height = height;
                global_win32_graphics_buffer.bytes_per_pixel = 4;
                global_win32_graphics_buffer.bytes_per_row = width*global_win32_graphics_buffer.bytes_per_pixel;

                int bitmap_memory_size = global_win32_graphics_buffer.width * global_win32_graphics_buffer.height * global_win32_graphics_buffer.bytes_per_pixel;
                global_win32_graphics_buffer.memory = VirtualAlloc(0, bitmap_memory_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

                global_win32_graphics_buffer.win32_dib_info.bmiHeader.biSize = sizeof(global_win32_graphics_buffer.win32_dib_info.bmiHeader);
                global_win32_graphics_buffer.win32_dib_info.bmiHeader.biWidth = global_win32_graphics_buffer.width;
                global_win32_graphics_buffer.win32_dib_info.bmiHeader.biHeight = -global_win32_graphics_buffer.height;
                global_win32_graphics_buffer.win32_dib_info.bmiHeader.biPlanes = 1;
                global_win32_graphics_buffer.win32_dib_info.bmiHeader.biBitCount = 32;
                global_win32_graphics_buffer.win32_dib_info.bmiHeader.biCompression = BI_RGB;
            }

// NOTE(ian): If we're doing a debug-build, then we want to have a consistent
// base address for our memory block, which will make debugging easier.
#if GOL_DEBUG
            LPVOID base_address = (LPVOID)Terabytes(2);
#else
            LPVOID base_address = 0;
#endif

            // ALLOCATE GAME MEMORY
            game_memory memory    = {};
            memory.is_initialized = false;
            memory.storage_size   = Megabytes(64);
            memory.used           = 0;
            memory.storage_memory = VirtualAlloc(base_address,
                                                 (size_t)memory.storage_size,
                                                 MEM_RESERVE|MEM_COMMIT,
                                                 PAGE_READWRITE);

            game_input inputs[2] = {};
            game_input *old_input = &inputs[0]; // input for the previous frame
            game_input *new_input = &inputs[1]; // input for the current frame
            new_input->scaling_factor = 1;
            new_input->animation_speed_factor = 1.0f;


            // LOCK THE FRAME RATE
            // TODO(casey): How do we reliably query on this on Windows?
            f32 target_seconds_per_frame = 0.0f;
            {
                int monitor_refresh_hz = 60;
                HDC refresh_dc = GetDC(Window);
                int win32_refresh_rate = GetDeviceCaps(refresh_dc, VREFRESH);
                ReleaseDC(Window, refresh_dc);
                if(win32_refresh_rate > 1)
                {
                    monitor_refresh_hz = win32_refresh_rate;
                }
                f32 game_update_hz = (monitor_refresh_hz / 2.0f);
                target_seconds_per_frame = 1.0f / game_update_hz;
            }

            while(global_running)
            {
                LARGE_INTEGER start_frame = win32_get_wall_clock();

                POINT mouse_pos;
                GetCursorPos(&mouse_pos);
                ScreenToClient(Window, &mouse_pos);
                new_input->mouse_x = mouse_pos.x;
                new_input->mouse_y = mouse_pos.y;

                new_input->mouse_left = GetKeyState(VK_LBUTTON) & (1 << 15);
                new_input->mouse_right = GetKeyState(VK_RBUTTON) & (1 << 15);
#if 1
                char debug_str[256];
                _snprintf_s(debug_str, sizeof(debug_str),
                            "x: %d y: %d left: %d, right: %d\n",
                            new_input->mouse_x, new_input->mouse_y,
                            new_input->mouse_left, new_input->mouse_right);
                OutputDebugStringA(debug_str);
#endif
                Assert(new_input->scaling_factor > 0);
                win32_process_pending_messages(new_input);

                if(!global_pause)
                {
                    // NOTE(ian): Since I don't want the game to know about the OS,
                    // here we must we convert from a platform-specific graphics buffer
                    // (i.e. a win32_graphics_buffer) to a platform-independent buffer.
                    game_graphics_buffer graphics_buffer = {};
                    graphics_buffer.memory = global_win32_graphics_buffer.memory;
                    graphics_buffer.width = global_win32_graphics_buffer.width;
                    graphics_buffer.height = global_win32_graphics_buffer.height;
                    graphics_buffer.bytes_per_row = global_win32_graphics_buffer.bytes_per_row;
                    graphics_buffer.bytes_per_pixel = global_win32_graphics_buffer.bytes_per_pixel;


                    if(new_input->reset)
                    {
                        memory.is_initialized = false;
                        memory.used           = 0;
                    }
                    game_update_and_render(&graphics_buffer, &memory, *new_input, *old_input);

                    for(int i = 0;
                        i < Array_Count(new_input->button_states);
                        i += 1)
                    {
                        old_input->button_states[i] = new_input->button_states[i];
                    }

#if 0

                    char debug_input_str[256];
                    _snprintf_s(debug_input_str, sizeof(debug_input_str),
                                "new right: %d old_right: %d\n",
                                new_input->right, old_input->right);
                    OutputDebugStringA(debug_input_str);
#endif

                    win32_window_dimension dimension = win32_get_window_dimension(Window);
                    HDC device_context = GetDC(Window);

                    win32_display_buffer_in_window(&global_win32_graphics_buffer,
                                                   device_context,
                                                   dimension);

                    ReleaseDC(Window, device_context);

                    // LOCK FRAME RATE
                    {
                        if(new_input->run_simulation)
                        {
                            target_seconds_per_frame = 0.25f * new_input->animation_speed_factor;
                        }
                        else
                        {
                            {
                                int monitor_refresh_hz = 60;
                                HDC refresh_dc = GetDC(Window);
                                int win32_refresh_rate = GetDeviceCaps(refresh_dc, VREFRESH);
                                ReleaseDC(Window, refresh_dc);
                                if(win32_refresh_rate > 1)
                                {
                                    monitor_refresh_hz = win32_refresh_rate;
                                }
                                f32 game_update_hz = (monitor_refresh_hz / 2.0f);
                                target_seconds_per_frame = 1.0f / game_update_hz;
                            }
                        }
                        LARGE_INTEGER end_frame = win32_get_wall_clock();
                        f32 seconds_elapsed = win32_get_seconds_elapsed(performance_counter_frequency,
                                                                        start_frame, end_frame);

                        DWORD ms_remaining = 0;
                        if(seconds_elapsed < target_seconds_per_frame)
                        {
                            ms_remaining = (DWORD)(1000.0f * (target_seconds_per_frame - seconds_elapsed));
                            if(ms_remaining > 0 && global_running)
                            {
                                Sleep(ms_remaining);
                            }
                        }
#if 0
                        f32 fps = 1.0f / (seconds_elapsed + ((f32)ms_remaining) / 1000.0f);
                        char debug_fps_str[256];
                        _snprintf_s(debug_fps_str, sizeof(debug_fps_str),
                                    "fps: %.02ff/s\n", fps);
                        OutputDebugStringA(debug_fps_str);
#endif
                    }
                }

            }
        }
        else
        {
            // TODO(ian): error logging!
        }
    }
    else
    {
        // TODO(ian): error logging!
    }

    return 0;
}
