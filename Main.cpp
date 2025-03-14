//--------------------------------------------------------------------------------------
// Entry point for the application
// Window creation code
//--------------------------------------------------------------------------------------
// Mostly old-school Windows code but also contains the outer game loop (Update Scene->Render Scene->repeat)

#include "SceneGlobals.h"
#include "DXDevice.h"
#include "RenderGlobals.h"
#include "Input.h"
#include "Timer.h"

#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

#include <windows.h>
#define _CRTDBG_MAP_ALLOC // To allow checking for memory leaks (see last lines of wWinMain)
//#include <stdlib.h>
#include <crtdbg.h>

#include <string>
#include <memory>


// Forward declarations of functions in this file
HWND InitWindow(HINSTANCE hInstance, int nCmdShow, int windowWidth, int windowHeight);
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
void UpdateWindowTitle(HWND hwnd, float frameTime);

// Global variable to determine if we are using mouse control for camera or not - can toggle with F9 if it is interfering with debugging etc.
bool gMouseControl = false;


//--------------------------------------------------------------------------------------
// The entry function for a Windows application is called wWinMain
//--------------------------------------------------------------------------------------
int APIENTRY wWinMain(_In_ HINSTANCE hInstance, [[maybe_unused]] _In_opt_ HINSTANCE hPrevInstance, 
                      [[maybe_unused]] _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
                      // [[maybe_unused]] is a C++17 feature that prevents warnings when variables (and other things) are declared but not used
{
    // Create a window to display the scene
    int windowWidth = 1280;
    int windowHeight = 720;
    HWND hWnd = InitWindow(hInstance, nCmdShow, windowWidth, windowHeight);
    if (hWnd == NULL)
    {
        MessageBoxA(hWnd, "Failure creating window", NULL, MB_OK);
        return 0;
    }

    // Initialise Direct3D and the demo scene
    try
    {
        DX = std::make_unique<DXDevice>(hWnd);
        gScene = std::make_unique<Scene>();
    }
    catch (std::runtime_error e)  // Constructors cannot return error messages so use exceptions to catch errors
    {
        MessageBoxA(hWnd, e.what(), NULL, MB_OK);
        return 0;
    }

    
    // Prepare TL-Engine style input functions
    InitInput();

    // Register for raw mouse input - allows mouse camera movement to work at the same time as a normal mouse cursor
    RAWINPUTDEVICE Rid[1];
    Rid[0].usUsagePage = 0x01;          // HID_USAGE_PAGE_GENERIC
    Rid[0].usUsage = 0x02;              // HID_USAGE_GENERIC_MOUSE
    Rid[0].dwFlags = 0;
    Rid[0].hwndTarget = 0;
    if (RegisterRawInputDevices(Rid, 1, sizeof(Rid[0])) == FALSE)  return 0;
    SetCursor(LoadCursor(NULL, IDC_ARROW));
    gMouseControl = true;

    //*******************************
    // Initialise ImGui
    //*******************************

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsClassic();

    // Setup Platform/Renderer bindings
    ImGui_ImplWin32_Init(hWnd); // Pass the window handle
    ImGui_ImplDX11_Init(DX->Device(), DX->Context());

    // Start a timer for frame timing
    Timer timer;
    timer.Start();

    // Main message loop
    MSG msg = {};
    while (msg.message != WM_QUIT) // As long as window is open
    {
        // Check for and deal with any window messages (input, window resizing, minimizing, etc.).
        // The actual message processing happens in the function WndProc below
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            // Deal with messages
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        else // When no windows messages left to process then render & update our scene
        {
            // Update the scene by the amount of time since the last frame
            float frameTime = timer.GetLapTime();
            gScene->Update(frameTime);

            // Draw the scene
			gScene->Render();

            UpdateWindowTitle(hWnd, frameTime);

            // Toggle mouse capture
            if (KeyHit(Key_F9))  gMouseControl = !gMouseControl;

            // This will close the window and trigger the WM_QUIT message that will cause this loop to end
            if (KeyHit(Key_Escape))  DestroyWindow(hWnd);
        }
    }

    //IMGUI
    //*******************************
    // Shutdown ImGui
    //*******************************

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    //*******************************

    // Output any memory leaks to Visual Studio's output window after the app ends
    _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);

    return (int)msg.wParam;
}



//--------------------------------------------------------------------------------------
// Create a window to display our scene, returns false on failure.
//--------------------------------------------------------------------------------------
HWND InitWindow(HINSTANCE hInstance, int nCmdShow, int windowWidth, int windowHeight)
{
    // Get a stock icon to show on the taskbar for this program.
    SHSTOCKICONINFO stockIcon;
    stockIcon.cbSize = sizeof(stockIcon);
    if (SHGetStockIconInfo(SIID_APPLICATION, SHGSI_ICON, &stockIcon) != S_OK)
    {
        return NULL;
    }

    // Register window class. Defines various UI features of the window for our application.
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;   // Which function deals with windows messages
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0; SIID_APPLICATION;
    wcex.hInstance = hInstance;
    wcex.hIcon = stockIcon.hIcon; // Which icon to use for the window
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW); // What cursor appears over the window
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = L"UCLANGamesWindowClass";
    wcex.hIconSm = stockIcon.hIcon;
    if (!RegisterClassEx(&wcex))
    {
        return NULL;
    }


    // Select the type of window to show our application in
    DWORD windowStyle = WS_OVERLAPPEDWINDOW; // Standard window
    //DWORD windowStyle = WS_POPUP;          // Alternative: borderless. If you also set the viewport size to the monitor resolution, you 
                                             // get a "fullscreen borderless" window, which works better with alt-tab than DirectX fullscreen,

    // Calculate overall dimensions for the window. We will render to the *inside* of the window. But the
    // overall winder will be larger if it includes borders, title bar etc. This code calculates the overall
    // size of the window given our choice of viewport size.
    RECT rc = { 0, 0, windowWidth, windowHeight };
    AdjustWindowRect(&rc, windowStyle, FALSE);

    // Create window, the second parameter is the text that appears in the title bar at first
    HWND hwnd = CreateWindow(L"UCLANGamesWindowClass", L"Direct3D 11", windowStyle,
                             CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance, nullptr);
    if (hwnd != NULL)
    {
        ShowWindow(hwnd, nCmdShow);
        UpdateWindow(hwnd);
    }

    return hwnd;
}


// Update window title with frame time / fps, averaged over a few fraems
void UpdateWindowTitle(HWND hwnd, float frameTime)
{
    // Show frame time / FPS in the window title
    const float fpsUpdateTime = 0.5f; // How long between window title updates (in seconds)
    static float totalFrameTime = 0;
    static int frameCount = 0;
    totalFrameTime += frameTime;
    ++frameCount;
    if (totalFrameTime > fpsUpdateTime)
    {
        // Displays FPS rounded to nearest int, and frame time (more useful for developers) in milliseconds to 2 decimal places
        float avgFrameTime = totalFrameTime / frameCount;
        std::ostringstream frameTimeMs;
        frameTimeMs.precision(2);
        frameTimeMs << std::fixed << avgFrameTime * 1000;
        std::string windowTitle = "CO3301 Boats Assignment - Frame Time: " + frameTimeMs.str() +
            "ms, FPS: " + std::to_string(static_cast<int>(1 / avgFrameTime + 0.5f));
        SetWindowTextA(hwnd, windowTitle.c_str());
        totalFrameTime = 0;
        frameCount = 0;
    }
}


//--------------------------------------------------------------------------------------
// Deal with a message from Windows. There are very many possible messages, such as keyboard/mouse input, resizing
// or minimizing windows, the system shutting down etc. We only deal with messages that we are interested in
//--------------------------------------------------------------------------------------
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam); //IMGUI add this line to support the line below
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam)) // IMGUI this line passes user input to ImGUI
        return true;

    switch (message)
    {
    case WM_PAINT: // A message we must handle to ensure the window content is displayed
    {
        PAINTSTRUCT ps;
        BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
    }
    break;

    case WM_DESTROY: // A message to deal with the window being closed
        PostQuitMessage(0);
        break;


    // The WM_KEYXXXX messages report keyboard input to our window.
    // This application has added some simple functions (not DirectX) to process these messages (all in Input.cpp/h)
    // so you don't need to change this code. Instead simply use KeyHit, KeyHeld etc.
    case WM_KEYDOWN:
        KeyDownEvent(static_cast<KeyCode>(wParam));
        break;

    case WM_KEYUP:
        KeyUpEvent(static_cast<KeyCode>(wParam));
        break;


    // The following WM_XXXX messages report mouse movement and button presses
    // Use KeyHit to get mouse buttons, GetMouse for its position
    case WM_LBUTTONDOWN:
    {
        KeyDownEvent(Mouse_LButton);
        break;
    }
    case WM_LBUTTONUP:
    {
        KeyUpEvent(Mouse_LButton);
        break;
    }
    case WM_RBUTTONDOWN:
    {
        KeyDownEvent(Mouse_RButton);
        break;
    }
    case WM_RBUTTONUP:
    {
        KeyUpEvent(Mouse_RButton);
        break;
    }
    case WM_MBUTTONDOWN:
    {
        KeyDownEvent(Mouse_MButton);
        break;
    }
    case WM_MBUTTONUP:
    {
        KeyUpEvent(Mouse_MButton);
        break;
    }
    // Using raw input for mouse movement so we can still get mouse movement even after the mouse cursor hits the edge of the scree
    case WM_INPUT:
    {
        
        UINT dwSize;
        RAWINPUTHEADER header;
        GetRawInputData((HRAWINPUT)lParam, RID_HEADER, &header, &dwSize, sizeof(RAWINPUTHEADER));
        if (header.dwType == RIM_TYPEMOUSE)
        {
            RAWINPUT rawInput;
            dwSize = sizeof(RAWINPUT);
            GetRawInputData((HRAWINPUT)lParam, RID_INPUT, &rawInput, &dwSize, sizeof(RAWINPUTHEADER));
            if (gMouseControl)  MouseMoveEvent(rawInput.data.mouse.lLastX, rawInput.data.mouse.lLastY);
        }
        break;
    }

    // Add to your window procedure
    case WM_MOUSEMOVE:
    {
        unsigned int x = MAKEPOINTS(lParam).x;
        unsigned int y = MAKEPOINTS(lParam).y;
        if (gMouseControl) {
            MouseGetEvent(x, y);
        }
        break;
    }

    // Any messages we don't handle are passed back to Windows default handling
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}
