#define global static
#define internal static

#ifndef UNICODE
#  define UNICODE
#endif

#include <windows.h>
#include "drawing.h"

#if !defined(__cplusplus)
#  if defined(_MSC_VER) && (_MSC_VER >= 1800)
#    include <stdbool.h>
#  elif defined(_MSC_VER)
#    ifndef __bool_true_false_are_defined
       typedef unsigned char bool;
#      define true  1
#      define false 0
#      define __bool_true_false_are_defined 1
#    endif
#  elif (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L) || defined(__GNUC__) || defined(__clang__)
#    include <stdbool.h>
#  else
#    ifndef __bool_true_false_are_defined
       typedef unsigned char bool;
#      define true  1
#      define false 0
#      define __bool_true_false_are_defined 1
#    endif
#  endif
#endif

typedef struct {
    BITMAPINFO info;
    HBITMAP handle;
    HDC device_context;
    OffscreenBuffer buffer;
} WIN32_OffscreenBuffer;

global bool global_running = true;
global WIN32_OffscreenBuffer global_backbuffer;
global const int BYTES_PER_PIXEL = 4;

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
    win32_offscreenbuffer->info.bmiHeader.biHeight      = -height; // Top-down DIB
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

LRESULT CALLBACK win32_window_proc(HWND windowHandle, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_KEYDOWN:
        {
            if (wParam == 'Q' || wParam == VK_ESCAPE)
            {
                DestroyWindow(windowHandle);
            }
        } return 0;

        case WM_DESTROY:
        {
            global_running = false;
        } return 0;

        case WM_PAINT:
        {
            PAINTSTRUCT paint;
            HDC deviceContext = BeginPaint(windowHandle, &paint);

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

            EndPaint(windowHandle, &paint);
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
            return DefWindowProc(windowHandle, msg, wParam, lParam);
    }
}

int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR pCmdLine,
    int nCmdShow
)
{
    const wchar_t WINDOW_CLASS_NAME[] = L"GDI_Demo_Window_Class";
    HWND windowHandle = NULL;
    MSG msg = {0};
    WNDCLASSEX windowClass = {0};

    int x_offset = 0;
    int y_offset = 0;

    /* Statements start here */
    (void)hPrevInstance;
    (void)pCmdLine;

    windowClass.cbSize        = sizeof(WNDCLASSEX);
    windowClass.lpfnWndProc   = win32_window_proc;
    windowClass.hInstance     = hInstance;
    windowClass.lpszClassName = WINDOW_CLASS_NAME;
    windowClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
    windowClass.hIcon         = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassEx(&windowClass))
    {
        MessageBoxW(NULL, L"Window Registration Failed!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
        return -1;
    }

    global_backbuffer.device_context = CreateCompatibleDC(NULL);

    windowHandle = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        WINDOW_CLASS_NAME,
        L"The title of my window",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        1024, 768,
        NULL, NULL, hInstance, NULL
    );

    if (windowHandle == NULL)
    {
        MessageBoxW(NULL, L"Window Creation Failed!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
        DeleteDC(global_backbuffer.device_context);
        return -1;
    }

    ShowWindow(windowHandle, nCmdShow);

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

        InvalidateRect(windowHandle, NULL, FALSE);
        UpdateWindow(windowHandle);
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
