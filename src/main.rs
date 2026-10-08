use std::mem::size_of;
use std::ptr::{null, null_mut};
use std::sync::atomic::{AtomicBool, Ordering};

use windows::core::{w, PCWSTR};
use windows::Win32::Foundation as win32_base;
use windows::Win32::Graphics::Gdi as win32_gdi;
use windows::Win32::System::LibraryLoader as win32_system;
use windows::Win32::UI::Input::KeyboardAndMouse as win32_input;
use windows::Win32::UI::WindowsAndMessaging as win32_ui;

mod win32 {
    pub use windows::Win32::Foundation::*;
    pub use windows::Win32::Graphics::Gdi::*;
    pub use windows::Win32::System::LibraryLoader::*;
    pub use windows::Win32::UI::Input::KeyboardAndMouse::*;
    pub use windows::Win32::UI::WindowsAndMessaging::*;
}

struct OffscreenBuffer {
    width: i32,
    height: i32,
    pitch: i32,
    pixels: *mut u32,
}

struct Win32_OffscreenBuffer {
    info: win32_gdi::BITMAPINFO,
    handle: win32_gdi::HBITMAP,
    device_context: win32_gdi::HDC,
    buffer: OffscreenBuffer,
}

// Global variables
static mut RUNNING: bool = true;

unsafe impl Sync for OffscreenBuffer {}
unsafe impl Sync for Win32_OffscreenBuffer {}

static mut GLOBAL_BACKBUFFER: Win32_OffscreenBuffer = Win32_OffscreenBuffer {
    info: win32_gdi::BITMAPINFO {
        bmiHeader: win32_gdi::BITMAPINFOHEADER {
            biSize: 0,
            biWidth: 0,
            biHeight: 0,
            biPlanes: 0,
            biBitCount: 0,
            biCompression: 0,
            biSizeImage: 0,
            biXPelsPerMeter: 0,
            biYPelsPerMeter: 0,
            biClrUsed: 0,
            biClrImportant: 0,
        },
        bmiColors: [win32_gdi::RGBQUAD {
            rgbBlue: 0,
            rgbGreen: 0,
            rgbRed: 0,
            rgbReserved: 0,
        }; 1],
    },
    handle: win32_gdi::HBITMAP(null_mut()),
    device_context: win32_gdi::HDC(null_mut()),
    buffer: OffscreenBuffer {
        width: 0,
        height: 0,
        pitch: 0,
        pixels: null_mut(),
    },
};

unsafe extern "system" fn win32_window_proc(
    window_handle: win32_base::HWND,
    msg: u32,
    wparam: win32_base::WPARAM,
    lparam: win32_base::LPARAM,
) -> win32_base::LRESULT {
    match msg {
        win32_ui::WM_KEYDOWN => {
            if wparam.0 == b'Q' as usize || wparam.0 == win32_input::VK_ESCAPE.0 as usize {
                let _ = win32::DestroyWindow(window_handle);
            }
            win32_base::LRESULT(0)
        }

        win32_ui::WM_DESTROY => {
            RUNNING = false;
            win32::PostQuitMessage(0);
            win32_base::LRESULT(0)
        }

        win32_ui::WM_PAINT => win32_base::LRESULT(0),

        win32_ui::WM_SIZE => win32_base::LRESULT(0),

        _ => win32::DefWindowProcW(window_handle, msg, wparam, lparam),
    }
}

fn main() -> windows::core::Result<()> {
    let instance = unsafe { win32::GetModuleHandleW(None)? };

    let window_class = win32_ui::WNDCLASSEXW {
        cbSize: size_of::<win32_ui::WNDCLASSEXW>() as u32,
        lpfnWndProc: Some(win32_window_proc),
        hInstance: instance.into(),
        lpszClassName: w!("MyWindowClass"),
        hCursor: unsafe { win32::LoadCursorW(None, win32_ui::IDC_ARROW).unwrap_or_default() },
        hIcon: unsafe { win32::LoadIconW(None, win32_ui::IDI_APPLICATION).unwrap_or_default() },
        ..Default::default()
    };

    let atom = unsafe { win32::RegisterClassExW(&window_class) };
    if atom == 0 {
        unsafe {
            win32::MessageBoxW(
                None,
                w!("Window Registration Failed!"),
                w!("Error"),
                win32_ui::MB_ICONEXCLAMATION | win32_ui::MB_OK,
            );
        }
        return Ok(());
    }

    unsafe {
        GLOBAL_BACKBUFFER.device_context = win32::CreateCompatibleDC(None);
    }

    let window_handle = unsafe {
        win32::CreateWindowExW(
            win32_ui::WS_EX_CLIENTEDGE,
            w!("MyWindowClass"),
            w!("The title of my window"),
            win32_ui::WS_OVERLAPPEDWINDOW,
            win32_ui::CW_USEDEFAULT,
            win32_ui::CW_USEDEFAULT,
            1024,
            768,
            None,
            None,
            win32_base::HINSTANCE(instance.0),
            Some(null()),
        )?
    };

    if window_handle.0.is_null() {
        unsafe {
            win32::MessageBoxW(
                None,
                w!("Window Creation Failed!"),
                w!("Error"),
                win32_ui::MB_ICONEXCLAMATION | win32_ui::MB_OK,
            );

            win32::DeleteDC(GLOBAL_BACKBUFFER.device_context).expect("Failed to delete backbuffer's device context");
        }
        return Ok(());
    }

    unsafe {
        let _ = win32::ShowWindow(window_handle, win32_ui::SW_SHOW);
    }

    let mut msg = win32_ui::MSG::default();

    while unsafe { RUNNING } {
        unsafe {
            while win32::PeekMessageW(&mut msg, None, 0, 0, win32_ui::PM_REMOVE).as_bool() {
                if msg.message == win32_ui::WM_QUIT {
                    RUNNING = false;
                }

                let _ = win32::TranslateMessage(&msg);
                win32::DispatchMessageW(&msg);
            }

            win32::InvalidateRect(window_handle, None, false).expect("Failed to invalidate the region");
            win32::UpdateWindow(window_handle)
                .ok()
                .expect("Failed to update the window");
        }
    }

    unsafe {
        if !GLOBAL_BACKBUFFER.handle.is_invalid() {
           win32::DeleteObject(GLOBAL_BACKBUFFER.handle).expect("Failed to delete backbuffer HBITMAP");
        }
        if !GLOBAL_BACKBUFFER.device_context.is_invalid() {
            win32::DeleteDC(GLOBAL_BACKBUFFER.device_context).expect("Failed to delete backbuffer device context");
        }
    }

    Ok(())
}
