#include <vlib/vlib.h>

// internal headers
#include "editor.h"
#include "data_structures.h"
#include "input.h"

#include "editor.cpp"	 // NOTE: в идеале должен компилироваться отдельно, т.к. отдельный модуль

// external headers
#include <imgui/imgui.h>
#include <imgui/imgui_impl_opengl3.h>

// win32
#include <windows.h>
#include <GL/gl.h>
#include <GL/wglext.h>
#include <imgui/imgui_impl_win32.h>

// extern "C" int _fltused = 0x9875; // чтобы работал float

// void __security_check_cookie(uintptr_t cookie) { } // TODO: выяснить зачем это нужно

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WindowProc(HWND window, UINT msg, WPARAM wParam, LPARAM lParam);

//
// Global state
//

static bool g_ProgramRunning;
static event_queue g_EventQueue;

static program_input inputs[2];
static program_input* oldInput = &inputs[0];
static program_input* newInput = &inputs[1];

static HANDLE g_Console;
static HDC g_DeviceContext;
static HGLRC g_OglContext;
static HWND g_Window;

void ErrorHandle(const char* msg = NULL) {
	DWORD errorCode = GetLastError();
	platform_Print("\nERROR");
	if (msg)
		platform_Print(msg);
	ExitProcess(EXIT_FAILURE);
}

// map windows virtual key to internal repr
te_Key MapKey(u32 virtualKey) {
	if (virtualKey >= '0' && virtualKey <= '9')
		return (te_Key)virtualKey;
	if (virtualKey >= 'A' && virtualKey <= 'Z')
		return (te_Key)virtualKey;

	switch (virtualKey) {
		case VK_LEFT:
			return Key_ArrowLeft;
		case VK_RIGHT:
			return Key_ArrowRight;
		case VK_UP:
			return Key_ArrowUp;
		case VK_DOWN:
			return Key_ArrowDown;

		case VK_SHIFT:
		case VK_LSHIFT:
		case VK_RSHIFT:
			return Key_Shift;
		case VK_CONTROL:
		case VK_LCONTROL:
		case VK_RCONTROL:
			return Key_Ctrl;
		case VK_MENU:
		case VK_LMENU:
		case VK_RMENU:
			return Key_Alt;

		case VK_ESCAPE:
			return Key_Esc;
		case VK_RETURN:
			return Key_Enter;
	}

	return Key_None;
}

//
// Platform API
//

void platform_Print(const char* msg) {
	if (!g_Console)
		g_Console = GetStdHandle(STD_OUTPUT_HANDLE);
	WriteConsoleA(g_Console, msg, strlen(msg), NULL, 0);
}

void platform_StartFrame() {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void platform_EndFrame() {
	ImGuiIO& io = ImGui::GetIO();
	glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
	glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	SwapBuffers(g_DeviceContext);
}

void* platform_debug_Malloc(s64 size) {
	return VirtualAlloc(0, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
}

void platform_debug_Free(void* mem) {
	if (mem)
		VirtualFree(mem, 0, MEM_RELEASE);
}

void* platform_debug_Realloc(void* oldMem, s64 oldSize, s64 newSize) {
	void* newMem = platform_debug_Malloc(newSize);
	MemCopy(newMem, oldMem, oldSize);
	platform_debug_Free(oldMem);
	return newMem;
}

void* MemReserve(s64 size) {
	return VirtualAlloc(0, size, MEM_RESERVE, PAGE_READWRITE);
}

void MemCommit(void* memory, s64 size) {
	VirtualAlloc(memory, size, MEM_COMMIT, PAGE_READWRITE);
}

//
// Main
//

int WinMain(
		HINSTANCE Instance,
		HINSTANCE hPrevInstance,
		LPSTR lpCmdLine,
		int nShowCmd) {
	WNDCLASSW wc = {};
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc = (WNDPROC)WindowProc;
	wc.hInstance = Instance;
	wc.lpszClassName = L"TextEditorWindowClass";

	s32 windowWidth = 900;
	s32 windowHeight = 600;

	// register class and create window
	if (RegisterClassW(&wc)) {
		g_Window = CreateWindowExW(
				0,
				wc.lpszClassName,
				L"Text Editor",
				WS_OVERLAPPEDWINDOW | WS_VISIBLE,
				CW_USEDEFAULT,
				CW_USEDEFAULT,
				windowWidth,
				windowHeight,
				0,
				0,
				Instance,
				0);

		if (g_Window) {
			//
			// Init OpenGL
			//

			g_DeviceContext = GetDC(g_Window);
			PIXELFORMATDESCRIPTOR pfd = {
					sizeof(PIXELFORMATDESCRIPTOR),
					1,
					PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
					PFD_TYPE_RGBA,
					32,	 // Цветовой буфер
					0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0,
					24,	 // Глубина буфера
					8,	 // Буфер трафарета
					0,
					PFD_MAIN_PLANE,
					0,
					0, 0, 0};
			int pixelFormat = ChoosePixelFormat(g_DeviceContext, &pfd);
			SetPixelFormat(g_DeviceContext, pixelFormat, &pfd);

			HGLRC oglTempContext = wglCreateContext(g_DeviceContext);
			if (!oglTempContext)
				ErrorHandle("wglCreateContext()\n");

			wglMakeCurrent(g_DeviceContext, oglTempContext);

			PFNWGLCREATECONTEXTATTRIBSARBPROC func =
					(PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
			if (!func)
				ErrorHandle("wglGetProcAddress()");

			// ogl v3.3
			int oglAttributes[] = {
					WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
					WGL_CONTEXT_MINOR_VERSION_ARB, 3,
					WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
					0};
			g_OglContext = func(g_DeviceContext, 0, oglAttributes);

			wglMakeCurrent(NULL, NULL);
			wglDeleteContext(oglTempContext);
			wglMakeCurrent(g_DeviceContext, g_OglContext);

			//
			// Init Imgui
			//

			const char* glsl_version = "#version 330";
			IMGUI_CHECKVERSION();
			auto imguiContext = ImGui::CreateContext();
			// ImGui::SetCurrentContext(imguiContext);
			ImGuiIO& io = ImGui::GetIO();
			io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;			 // docking
			io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;	 // keyboard navigation
			// ImGui::StyleColorsDark();
			ImGui_ImplWin32_Init(g_Window);
			ImGui_ImplOpenGL3_Init(glsl_version);

			//
			// Init state
			//

			u64 eventQueueCapacity = Megabytes(5);

			g_EventQueue = EventQueue(platform_debug_Malloc(eventQueueCapacity), eventQueueCapacity);
			g_ProgramRunning = true;

			//
			// Main loop
			//

			SetForegroundWindow(g_Window);

			while (g_ProgramRunning) {
				//
				// Inputs
				//

				Clear(&g_EventQueue);	 // подгатавливаем очередь к WindowProc
				MemCopy(oldInput, newInput, sizeof(*newInput));
				newInput->keys[Key_WheelDown] = {0};
				newInput->keys[Key_WheelUp] = {0};

				MSG message;
				// NOTE: в hwnd нужен NULL, иначе обработка сообщений начинает работать неправильно
				// (не работал ALT + SHIFT, когда окно было в фокусе)
				while (PeekMessageW(&message, NULL, 0, 0, PM_REMOVE)) {
					TranslateMessage(&message);
					DispatchMessageW(&message);
				}

				for (size_t i = 0; i < ArrayCount(newInput->keys); i++) {
					ProcessButtonInput(&oldInput->keys[i], &newInput->keys[i], newInput->keys[i].isDown);
				}

				//
				// Editor Code
				//

				EditorUpdate(&g_EventQueue, newInput);
				EditorRender(newInput);
			}

			//
			// Shutdown Imgui
			//

			ImGui_ImplOpenGL3_Shutdown();
			ImGui_ImplWin32_Shutdown();
			ImGui::DestroyContext();

			//
			// Cleanup
			//

			// wglMakeCurrent(NULL, NULL);
			// wglDeleteContext(g_OglContext);
			// ReleaseDC(g_Window, g_DeviceContext);
			// DestroyWindow(g_Window);
		}
	}

	return EXIT_SUCCESS;
}

// Debug компилируем с флагом /subsystem:console
// Release с /subsystem:windows

// #if _DEBUG
// s32 __stdcall mainCRTStartup() {
// #else
// s32 __stdcall WinMainCRTStartup() {
// #endif
void main() {
	ThreadContextInit();

	s32 result = WinMain(GetModuleHandle(0), 0, 0, 0);
	ExitProcess(result);

	// ThreadContextRelease();
}

//
// Window callback
//

#define WAS_DOWN(lParam) ((lParam >> 30) & 1)

LRESULT CALLBACK WindowProc(HWND window, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (ImGui_ImplWin32_WndProcHandler(window, msg, wParam, lParam))
		return true;

	switch (msg) {
		case WM_DESTROY:
		case WM_CLOSE:
			g_ProgramRunning = false;
			return 0;

		case WM_INPUTLANGCHANGE:
			return 1;

		case WM_SYSKEYDOWN:
		case WM_KEYDOWN: {
			bool wasDown = WAS_DOWN(lParam);
			bool isDown = true;
			te_Key key = MapKey(wParam);

			key_event event = KeyEvent(key, wasDown, isDown);
			PUSH_EVENT(g_EventQueue, event);

			// ProcessButtonInput(&oldInput->keys[key], &newInput->keys[key], isDown);
			newInput->keys[key].isDown = isDown;
		} break;

		case WM_SYSKEYUP:
		case WM_KEYUP: {
			bool wasDown = WAS_DOWN(lParam);
			bool isDown = false;
			te_Key key = MapKey(wParam);

			key_event event = KeyEvent(key, wasDown, isDown);
			PUSH_EVENT(g_EventQueue, event);

			// ProcessButtonInput(&oldInput->keys[key], &newInput->keys[key], isDown);
			newInput->keys[key].isDown = isDown;
		} break;

		case WM_CHAR: {
			wchar_t wideChar = wParam;

			if (iswprint(wideChar))	 // CRT. TODO: можно ли убрать?
			{
				char utf8Buffer[5] = {0};
				s32 bytesWritten =
						WideCharToMultiByte(CP_UTF8, 0, &wideChar, 1, utf8Buffer, sizeof(utf8Buffer), NULL, NULL);

				// to little endian
				code_point utf8CodePoint = {0};
				for (s32 i = bytesWritten - 1, j = 0; i >= 0; i--, j++) {
					utf8CodePoint.bytes[j] = utf8Buffer[i];
				}

				bool wasDown = WAS_DOWN(lParam);

				// u16 repeatCount = lParam & 0xFFFF;

				char_event event = CharEvent(utf8CodePoint, wasDown, true);
				PUSH_EVENT(g_EventQueue, event);
			}
		}
			return 0;

		case WM_MOUSEWHEEL: {
			// newInput->wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
			s32 wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
			if (wheelDelta > 0) {
				newInput->keys[Key_WheelUp].isDown = true;
				newInput->keys[Key_WheelDown].isDown = false;
			} else {
				newInput->keys[Key_WheelDown].isDown = true;
				newInput->keys[Key_WheelUp].isDown = false;
			}
		}
			return 0;

		case WM_SIZE:
			return 0;

		case WM_PAINT: {
			EditorRender(newInput);	 // рендер на изменении размера окна

			PAINTSTRUCT paint;
			BeginPaint(window, &paint);
			EndPaint(window, &paint);
		}
			return 0;
	}

	return DefWindowProcW(window, msg, wParam, lParam);
	;
}

#include <vlib/vlib.cpp>