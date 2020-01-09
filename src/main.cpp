
#include <memory>
#include <cstdio>
#include <typeinfo>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "opengl_headers.h"

#include "opengl_helpers.h"
#include "maths.h"
#include "camera.h"
#include "platform.h"

#include "demo_minimal.h"
#include "demo_base.h"
#include "demo_pg_skybox.h"
#include "demo_postprocess.h"
// TODO(demo): Add headers here

#if 1
// Run on laptop high perf GPU
extern "C"
{
    __declspec(dllexport) int NvOptimusEnablement = 1;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

// Struct to store the state of the glfw keyboard
struct keyboard
{
    bool Keys[GLFW_KEY_LAST+1]; // glfw keys state (pressed or released)
};

// App state
struct app
{
    platform_io IO;
    GLFWwindow* Window;
    
    keyboard Keyboard = {};
};

// Update platform IO from glfw
void GLFWPlatformIOUpdate(GLFWwindow* Window, platform_io* IO)
{
    // Time calculation
    {
        double Time = glfwGetTime();
        double DeltaTime;
        if (IO->Time == 0.0)
            DeltaTime = 1.0 / 60.0;
        else
            DeltaTime = Time - IO->Time;

        IO->Time = Time;
        IO->DeltaTime = DeltaTime;
    }

    // Mouse state    
    {
        double MouseX, MouseY;
        glfwGetCursorPos(Window, &MouseX, &MouseY);

        IO->DeltaMouseX = (float)MouseX - IO->MouseX;
        IO->DeltaMouseY = (float)MouseY - IO->MouseY;

        IO->MouseX = (float)MouseX;
        IO->MouseY = (float)MouseY;
    }

    // Screen size
    glfwGetWindowSize(Window, &IO->ScreenWidth, &IO->ScreenHeight);
}

// Helper that translate platform IO to camera inputs
camera_inputs GLFWGetCameraInputs(const platform_io& IO, const keyboard& Keyboard)
{
    camera_inputs CameraInputs = {};

    // Camera movement
    if (IO.MouseCaptured)
    {
        const struct camera_key_mapping { int Key; int Flag; } CameraKeyMapping[]
        {
            { GLFW_KEY_W,            CAM_MOVE_FORWARD  },
            { GLFW_KEY_S,            CAM_MOVE_BACKWARD },
            { GLFW_KEY_A,            CAM_STRAFE_LEFT   },
            { GLFW_KEY_D,            CAM_STRAFE_RIGHT  },
            { GLFW_KEY_LEFT_SHIFT,   CAM_MOVE_FAST     },
            { GLFW_KEY_SPACE,        CAM_MOVE_UP       },
            { GLFW_KEY_LEFT_CONTROL, CAM_MOVE_DOWN     },
        };

        CameraInputs.DeltaTime = IO.DeltaTime;
        CameraInputs.MouseDX   = IO.DeltaMouseX;
        CameraInputs.MouseDY   = IO.DeltaMouseY;

        CameraInputs.KeyInputsMask = 0;
        for (int i = 0; i < (int)ARRAY_SIZE(CameraKeyMapping); ++i)
        {
            camera_key_mapping Map = CameraKeyMapping[i];
            CameraInputs.KeyInputsMask |= Keyboard.Keys[Map.Key] ? Map.Flag : 0;
        }
    }

    return CameraInputs;
}

// Callbacks
// ===========
void OpenGLErrorCallback(GLenum Source, GLenum Type, GLuint Id, GLenum Severity, GLsizei Length, const GLchar* Message, const void* UserParam)
{
    fprintf(stderr, "OpenGL log (0x%x): %s\n", Id, Message);
}

void GLFWErrorCallback(int ErrorCode, const char* Description)
{
    fprintf(stderr, "GLFW error (%d): %s\n", ErrorCode, Description);
}

void GLFWMouseButtonCallback(GLFWwindow* Window, int Button, int Action, int Mods)
{
    app* App = (app*)glfwGetWindowUserPointer(Window);

    if (Button == GLFW_MOUSE_BUTTON_RIGHT && Action == GLFW_PRESS)
    {
        App->IO.MouseCaptured = true;
        glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
}

void GLFWKeyCallback(GLFWwindow* Window, int Key, int Scancode, int Action, int Mods)
{
    app* App = (app*)glfwGetWindowUserPointer(Window);

    // Update keyboard
    if (Action == GLFW_PRESS)
        App->Keyboard.Keys[Key] = true;
    else if (Action == GLFW_RELEASE)
        App->Keyboard.Keys[Key] = false;
}

// Helper to check if a key has just been pressed
bool KeyPressed(int Key, const keyboard& PrevKeyboard, const keyboard& Keyboard)
{
    return (PrevKeyboard.Keys[Key] == false) && (Keyboard.Keys[Key] == true);
}

int main(int argc, char* argv[])
{
    const int WIDTH = 1440;
    const int HEIGHT = 900;
    
    app App = {};

    // Init GLFW
    glfwSetErrorCallback(GLFWErrorCallback);
    if (glfwInit() != GLFW_TRUE)
        return 1;

    // Create window
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
    { // Restricted scope to force access to Window with App.Window
        GLFWwindow* Window = glfwCreateWindow(WIDTH, HEIGHT, "Image Based rendering", nullptr, nullptr);
        glfwSetWindowUserPointer(Window, &App);
        App.Window = Window;
    }

    // Register GLFW callbacks
    glfwSetKeyCallback(App.Window, GLFWKeyCallback);
    glfwSetMouseButtonCallback(App.Window, GLFWMouseButtonCallback);

    // Init OpenGL
    glfwMakeContextCurrent(App.Window);
    glfwSwapInterval(1);
    if (!gladLoadGL())
    {
        fprintf(stderr, "gladLoadGL failed.\n");
        glfwTerminate();
        return 1;
    }

    // Setup KHR debug
    glDebugMessageCallback(OpenGLErrorCallback, nullptr);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
	glDebugMessageControl(GL_DONT_CARE, GL_DEBUG_TYPE_PERFORMANCE, GL_DONT_CARE, 0, nullptr, GL_FALSE);
	glDebugMessageControl(GL_DONT_CARE, GL_DEBUG_TYPE_OTHER, GL_DONT_CARE, 0, nullptr, GL_FALSE);
    
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    
    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(App.Window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    bool ShowDemoWindow = false;

    // Demo scope
    {
        // First update to pass to demo constructors
        GLFWPlatformIOUpdate(App.Window, &App.IO);

        int DemoId = 0; // Change this to start with another demo
        std::unique_ptr<demo> Demos[] = 
        {
            std::make_unique<demo_base>(),
            std::make_unique<demo_minimal>(),
            std::make_unique<demo_pg_skybox>(),
            std::make_unique<demo_postprocess>(App.IO),
            // TODO(demo): Add other demos here
        };

        // Main loop
        while (!glfwWindowShouldClose(App.Window))
        {
            // HANDLE INPUTS ------------------------------
            // --------------------------------------------
            // Store keyboard state before glfwPollEvents because input callbacks are triggered inside it
            keyboard PrevKeyboard = App.Keyboard;
            glfwPollEvents();

            GLFWPlatformIOUpdate(App.Window, &App.IO);
            
            App.IO.CameraInputs = GLFWGetCameraInputs(App.IO, App.Keyboard);

            // Escape key
            if (KeyPressed(GLFW_KEY_ESCAPE, PrevKeyboard, App.Keyboard))
            {
                if (App.IO.MouseCaptured)
                {
                    glfwSetInputMode(App.Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                    App.IO.MouseCaptured = false;
                }
                else
                {
                    glfwSetWindowShouldClose(App.Window, GLFW_TRUE);
                }
            }

            // Debug keys
            for (int i = 0; i < 12; ++i)
            {
                int Key = GLFW_KEY_F1 + i;
                App.IO.DebugKeysDown[i]    = App.Keyboard.Keys[Key];
                App.IO.DebugKeysPressed[i] = KeyPressed(Key, PrevKeyboard, App.Keyboard);
            }
            
            // Start the Dear ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            // Disabling mouse for ImGui if mouse is captured by the app (it must be done here)
            if (App.IO.MouseCaptured)
                ImGui::GetIO().MousePos = ImVec2(-FLT_MAX,-FLT_MAX);
            ImGui::NewFrame();
            
            // Demo id selector
            {
                if (ImGui::Button("Previous"))
                    DemoId = Math::TrueMod(DemoId - 1, (int)ARRAY_SIZE(Demos));
                ImGui::SameLine();
                ImGui::Text("%d/%d", DemoId+1, (int)ARRAY_SIZE(Demos));
                ImGui::SameLine();
                if (ImGui::Button("Next"))
                    DemoId = Math::TrueMod(DemoId + 1, (int)ARRAY_SIZE(Demos));
                ImGui::SameLine();
                ImGui::Text("[%s]", typeid(*Demos[DemoId]).name());
            }

            // Display GPU infos
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::Checkbox("Demo window", &ShowDemoWindow);

            if (ImGui::CollapsingHeader("System info"))
            {
                ImGui::Text("GL_VERSION: %s", glGetString(GL_VERSION));
                ImGui::Text("GL_RENDERER: %s", glGetString(GL_RENDERER));
                ImGui::Text("GL_SHADING_LANGUAGE_VERSION: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));
            }
            
            if (ShowDemoWindow)
                ImGui::ShowDemoWindow(&ShowDemoWindow);

            // Display demo
            Demos[DemoId]->Update(App.IO);

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            // Present framebuffer
            glfwSwapBuffers(App.Window);
        }
    }

    // Terminate glfw
    glfwDestroyWindow(App.Window);
    glfwTerminate();

    return 0;
}
