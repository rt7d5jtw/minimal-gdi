#include <windows.h>
#include "common.h"
#include "drawing.h"

typedef struct WIN32_OffscreenBuffer WIN32_OffscreenBuffer;
struct WIN32_OffscreenBuffer
{
    BITMAPINFO info;
    HBITMAP handle;
    HDC device_context;
    OffscreenBuffer buffer;
};

internal b32 win32_register_window_class(HINSTANCE h_instance);
internal b32 win32_create_window(HWND *window_handle, HINSTANCE h_instance, LPCWSTR window_class_name, LPCWSTR window_name, int window_width, int window_height);
internal void win32_resize_dib_section(WIN32_OffscreenBuffer *win32_offscreenbuffer, int width, int height);
LRESULT CALLBACK win32_window_proc(HWND windowHandle, UINT msg, WPARAM wParam, LPARAM lParam);

global bool global_running = true;
global WIN32_OffscreenBuffer global_backbuffer;
global const int BYTES_PER_PIXEL = 4;
global const wchar_t WINDOW_CLASS_NAME[] = L"GDI_Demo_Window_Class";

internal b32 win32_register_window_class(HINSTANCE h_instance)
{
  WNDCLASSEXW window_class = {0};

  window_class.cbSize        = sizeof(window_class);
  window_class.lpfnWndProc   = win32_window_proc;
  window_class.hInstance     = h_instance;
  window_class.lpszClassName = WINDOW_CLASS_NAME;
  window_class.hCursor       = LoadCursor(NULL, IDC_ARROW);
  window_class.hIcon         = LoadIcon(NULL, IDI_APPLICATION);

  return (RegisterClassExW(&window_class) != 0);
}

internal b32 win32_create_window(HWND *out_window_handle, HINSTANCE h_instance, LPCWSTR window_class_name, LPCWSTR window_name, int window_width, int window_height)
{
    DWORD extended_window_style    = WS_EX_CLIENTEDGE;
    DWORD window_style             = WS_OVERLAPPEDWINDOW;
    int window_x                   = CW_USEDEFAULT;
    int window_y                   = CW_USEDEFAULT;
    HWND window_parent             = NULL;
    HMENU window_menu              = NULL;
    LPVOID window_create_parameter = NULL;

    *out_window_handle = CreateWindowExW(
        extended_window_style,
        window_class_name,
        window_name,
        window_style,
        window_x,
        window_y,
        window_width,
        window_height,
        window_parent,
        window_menu,
        h_instance,
        window_create_parameter
    );

    return (*out_window_handle != NULL);
}

internal void win32_resize_dib_section(WIN32_OffscreenBuffer *win32_offscreenbuffer, int width, int height)
{
    if (win32_offscreenbuffer->handle)
    {
        DeleteObject(win32_offscreenbuffer->handle);
        win32_offscreenbuffer->handle = NULL;
    }

    win32_offscreenbuffer->buffer.width  = width;
    win32_offscreenbuffer->buffer.height = height;
    win32_offscreenbuffer->buffer.pitch  = width * BYTES_PER_PIXEL;

    win32_offscreenbuffer->info.bmiHeader.biSize        = sizeof(win32_offscreenbuffer->info.bmiHeader);
    win32_offscreenbuffer->info.bmiHeader.biWidth       = width;
    win32_offscreenbuffer->info.bmiHeader.biHeight      = -height; /* Top-down DIB */
    win32_offscreenbuffer->info.bmiHeader.biPlanes      = 1;
    win32_offscreenbuffer->info.bmiHeader.biBitCount    = 32;
    win32_offscreenbuffer->info.bmiHeader.biCompression = BI_RGB;

    win32_offscreenbuffer->handle = CreateDIBSection(
        win32_offscreenbuffer->device_context,
        &win32_offscreenbuffer->info,
        DIB_RGB_COLORS,
        (void **)&win32_offscreenbuffer->buffer.pixels,
        NULL,
        0
    );

    SelectObject(win32_offscreenbuffer->device_context, win32_offscreenbuffer->handle);
}

LRESULT CALLBACK win32_window_proc(HWND window_handle, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_KEYDOWN:
        {
            if (wParam == 'Q' || wParam == VK_ESCAPE)
            {
                DestroyWindow(window_handle);
            }
        } return 0;

        case WM_DESTROY:
        {
            global_running = false;
        } return 0;

        case WM_PAINT:
        {
            PAINTSTRUCT paint;
            HDC deviceContext = BeginPaint(window_handle, &paint);

            BitBlt(
                deviceContext,
                paint.rcPaint.left,
                paint.rcPaint.top,
                paint.rcPaint.right - paint.rcPaint.left,
                paint.rcPaint.bottom - paint.rcPaint.top,
                global_backbuffer.device_context,
                paint.rcPaint.left,
                paint.rcPaint.top,
                SRCCOPY
            );

            EndPaint(window_handle, &paint);
        } return 0;

        case WM_SIZE:
        {
            int new_width  = LOWORD(lParam);
            int new_height = HIWORD(lParam);
            if (new_width > 0 && new_height > 0)
            {
                win32_resize_dib_section(&global_backbuffer, new_width, new_height);
            }
        } return 0;

        default:
            return DefWindowProc(window_handle, msg, wParam, lParam);
    }
}

int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR pCmdLine,
    int nCmdShow
)
{
    MSG msg = {0};
    HWND window_handle = NULL;
    WNDCLASSEX windowClass = {0};

    int x_offset = 0;
    int y_offset = 0;

    /* Statements start here */
    (void)hPrevInstance;
    (void)pCmdLine;

    if (!win32_register_window_class(hInstance))
    {
        MessageBoxW(NULL, L"Window Registration Failed!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
        return 1;
    }

    if (!win32_create_window(&window_handle, hInstance, WINDOW_CLASS_NAME, L"GDI Demo", WINDOW_WIDTH, WINDOW_HEIGHT))
    {
        MessageBoxW(NULL, L"Window Creation Failed!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
        return 1;
    }

    global_backbuffer.device_context = CreateCompatibleDC(NULL);
    ShowWindow(window_handle, nCmdShow);

    while (global_running)
    {
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                global_running = false;
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        draw_gradient(&global_backbuffer.buffer, x_offset, y_offset);
        ++x_offset;

        InvalidateRect(window_handle, NULL, FALSE);
        UpdateWindow(window_handle);
    }

    if (global_backbuffer.handle)
    {
        DeleteObject(global_backbuffer.handle);
    }
    if (global_backbuffer.device_context)
    {
        DeleteDC(global_backbuffer.device_context);
    }

    return (int)msg.wParam;
}
