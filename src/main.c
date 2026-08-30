#include <stdbool.h>
#include <stdint.h>

#if defined(_WIN32)
#  include <windows.h>
#  ifndef UNICODE
#    define UNICODE
#  endif

/* Forward declaration */
LRESULT CALLBACK win32WndProc(HWND, UINT, WPARAM, LPARAM);

/* main loop */
static bool global_running = true;

static BITMAPINFO frameBitmapInfo;
static HBITMAP frameBitmap = 0;
static HDC frameDeviceContext = 0;
int bytes_per_pixel = 4;

// START OF GDI Drawing declarations {{{
struct {
  int width;
  int height;
  uint32_t *pixels; // pixel array for the bitmap
} frame = {0};

void draw_random_gradient(uint32_t *bitmap_memory, int bitmap_width, int bitmap_height, int x_offset, int y_offset) {
  int pitch = bitmap_width * bytes_per_pixel;
  uint8_t *row = (uint8_t *)bitmap_memory;

  for (int y = 0; y < bitmap_height; ++y) {
    uint8_t *pixel = (uint8_t *)row;
    for (int x = 0; x < bitmap_width; ++x) {
      // Blue channel
      *pixel = (uint8_t)(x + x_offset);
      ++pixel;

      // Green channel
      *pixel = (uint8_t)(y + y_offset);
      ++pixel;

      // Red channel
      *pixel = 0;
      ++pixel;

      // Padding or something idk ?
      *pixel = 0;
      ++pixel;
    }

    row += pitch;
  }
}

// Set each pixel in order to a random value one per frame while raising another random pixel to black
void draw_random_pixel_values(void) {
  // Additional Rand function to generate random pixels on the screen
  #if RAND_MAX == 32767
  #define Rand32() ((rand() << 16) + (rand() << 1) + (rand() & 1))
  #else
  #define Rand32() rand()
  #endif
  static unsigned int pixel = 0;
  frame.pixels[(pixel++) % (frame.width * frame.height)] = Rand32();
  frame.pixels[Rand32() % (frame.width * frame.height)] = 0;
}

/// }}}

/**
 * Entrypoint for Windows
 * https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-winmain
 * https://learn.microsoft.com/en-us/windows/win32/learnwin32/winmain--the-application-entry-point
 */
int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    PWSTR pCmdLine,
    int nCmdShow
)
{
  // WNDCLASS
  // https://learn.microsoft.com/en-us/previous-versions/ms942860(v=msdn.10)
  // WNDCLASSA
  // https://learn.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-wndclassa
  // WNDCLASSEXA
  // https://learn.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-wndclassexa

  // Contains window class information
  WNDCLASSEX windowClass = {0};

  // https://learn.microsoft.com/en-us/windows/win32/learnwin32/creating-a-window
  const wchar_t window_class_name[] = L"Sample Window Class";

  HWND windowHandle = NULL;
  static MSG msg = {0};

  windowClass.cbSize        = sizeof(WNDCLASSEX);
  windowClass.style         = 0;
  windowClass.lpszClassName = window_class_name;
  windowClass.lpfnWndProc   = win32WndProc; // Long Pointer to the Windows Procedure function
  windowClass.cbClsExtra    = 0;
  windowClass.cbWndExtra    = 0;
  windowClass.hInstance     = hInstance;
  windowClass.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
  windowClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
  windowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
  windowClass.lpszMenuName  = NULL;
  windowClass.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);

  if (!RegisterClassEx(&windowClass)) {
    // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-messagebox
    MessageBox(
        windowHandle,
        "Window Registration Failed!",
        "Error!",
        MB_ICONEXCLAMATION | MB_OK
    );

    return -1;
  }

  // GDI Drawing code initialization {{{

  // Dimensions and color information for the bitmap
  // https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-bitmapinfo
  // https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-bitmapinfoheader
  frameBitmapInfo.bmiHeader.biSize        = sizeof(frameBitmapInfo.bmiHeader); // the number of bytes required by the structure
  frameBitmapInfo.bmiHeader.biPlanes      = 1;                                 // the number of planes for the target device, must be set to 1
  frameBitmapInfo.bmiHeader.biBitCount    = 32;                                // the number of of bits per pixel (bpp)
  frameBitmapInfo.bmiHeader.biCompression = BI_RGB;                            // uncompressed RGB format

  // https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-createcompatibledc
  frameDeviceContext = CreateCompatibleDC(0);
  
  /// }}}

  // window parameters
  DWORD extended_window_style = WS_EX_CLIENTEDGE;
  LPCWSTR window_name         = "The title of my window";
  DWORD window_style          = WS_OVERLAPPEDWINDOW;
  int window_x                = CW_USEDEFAULT; // horizontal position of the window
  int window_y                = CW_USEDEFAULT; // vertical position of the window
  int window_width            = 1024;
  int window_height           = 768;
  HWND window_parent          = NULL;
  HMENU window_menu           = NULL;
  LPVOID lp_param             = NULL;

  // Window handle for
  windowHandle = CreateWindowEx(
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
    hInstance,
    lp_param
  );

  if (windowHandle == NULL) {
    MessageBox(
      windowHandle,
      "Window Creation Failed!", 
      "Error!", 
      MB_ICONEXCLAMATION | MB_OK
    );
    return GetLastError();
  }

  // ShowWindow
  // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-showwindow
  ShowWindow(windowHandle, nCmdShow);

  int x_offset = 0;
  int y_offset = 0;

  while (global_running) {
    // Run the message loop
    // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-peekmessagew
    // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-dispatchmessagea
    while (PeekMessageW(&msg, 0, 0, 0, PM_REMOVE)) {
      if (msg.message == WM_QUIT) { global_running = false; }
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }

    // GDI Drawing logic {{{

    //draw_random_pixel_values();
    draw_random_gradient(frame.pixels, frame.width, frame.height, x_offset, y_offset);
    ++x_offset;

    /*
     InvalidateRect marks a section of the window invalid and 
     needing to be redrawn. Passing in NULL invalidates the entire window.

     UpdateWindow immediately passes a WM_PAINT message to the 
     window process message function, rather than waiting for 
     the next message processing loop. This allows us to redraw 
     the window whenever we want rather than waiting for Windows to tell us to.
    */

    InvalidateRect(windowHandle, NULL, FALSE); // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-invalidaterect
    UpdateWindow(windowHandle);                // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-updatewindow
    // }}}
  }

  return msg.wParam;
}

/** Windows Message Callback function */
LRESULT CALLBACK
win32WndProc(HWND windowHandle, UINT msg, WPARAM wParam, LPARAM lParam)
{
  LRESULT result = 0;
  switch (msg) {
    case WM_KEYDOWN:
    {
      switch (wParam) {
        // Close window from 'Q'
        case 'Q':
        {
          DestroyWindow(windowHandle);
        }
      }
    } break;
    case WM_QUIT:
    case WM_DESTROY: {
      global_running = false;
    } break;
    // GDI Drawing logic {{{
    case WM_PAINT: {
      static PAINTSTRUCT paint;
      static HDC deviceContext;

      deviceContext = BeginPaint(windowHandle, &paint); // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-beginpaint

      // https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-bitblt
      // Painting function to copy the pixel array over to the window in the specified rectangle
      BitBlt(
        deviceContext,
        paint.rcPaint.left,
        paint.rcPaint.top,
        paint.rcPaint.right - paint.rcPaint.left,
        paint.rcPaint.bottom - paint.rcPaint.top,
        frameDeviceContext,
        paint.rcPaint.left,
        paint.rcPaint.top,
        SRCCOPY
      );

      EndPaint(windowHandle, &paint); // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-beginpaint
    } break;
    // Set the size of the pixel array and finish setting up GDI bitmap
    case WM_SIZE: {
      frameBitmapInfo.bmiHeader.biWidth  = LOWORD(lParam);
      frameBitmapInfo.bmiHeader.biHeight = HIWORD(lParam);

      // Delete already existing bitmap
      if (frameBitmap) DeleteObject(frameBitmap);

      // https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-createdibsection
      // Create a bitmap
      frameBitmap = CreateDIBSection(
        NULL,                    /* hdc      - Handle to a device context */
        &frameBitmapInfo,        /* pbmi     - Pointer to bitmap info */
        DIB_RGB_COLORS,          /* usage    - type of data contained in the bmiColors array member of the BITMAPINFO structure pointed to by pbmi */
        (void **)&frame.pixels,  /* ppvBits  - a pointer to a variable that receives a pointer ot the location of the DIB bit values */
         0,                      /* hSection - a handle to a file-mapping object that hte function will use to create the DIB. */
         0                       /* offset   - the offset form the beginning of the file-mapping object referenced by hSection where storage for the bitmap bit values is to begin */
        );
      
      // https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-selectobject
      // point device context to the bitmap
      SelectObject(frameDeviceContext, frameBitmap);

      frame.width  = LOWORD(lParam);
      frame.height = HIWORD(lParam);
    } break;
    /// }}}
    default: {
      result = DefWindowProc(windowHandle, msg, wParam, lParam);
    }
  }
  return result;
}
#endif