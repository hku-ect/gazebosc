// Clang/GCC warnings with -Weverything
#if defined(__clang__)
#pragma clang diagnostic push
#if __has_warning("-Wunknown-warning-option")
#pragma clang diagnostic ignored "-Wunknown-warning-option"         // warning: unknown warning group 'xxx'
#endif
#pragma clang diagnostic ignored "-Wunknown-pragmas"                // warning: unknown warning group 'xxx'
#pragma clang diagnostic ignored "-Wunused-function"                // for stb_textedit.h
#pragma clang diagnostic ignored "-Wmissing-prototypes"             // for stb_textedit.h
#pragma clang diagnostic ignored "-Wold-style-cast"
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"
#pragma clang diagnostic ignored "-Wdouble-promotion"
#pragma clang diagnostic ignored "-Wimplicit-int-float-conversion"  // warning: implicit conversion from 'xxx' to 'float' may lose precision
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpragmas"              // warning: unknown option after '#pragma GCC diagnostic' kind
#pragma GCC diagnostic ignored "-Wclass-memaccess"      // [__GNUC__ >= 8] warning: 'memset/memcpy' clearing/writing an object of type 'xxxx' with no trivial copy-assignment; use assignment or value-initialization instead
#endif

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"
#include "imgui_internal.h"
#include <stdio.h>
#define SDL_MAIN_HANDLED
#include <SDL.h>

#include <chrono>
#include <thread>

#include <signal.h>
#include "libsphactor.h"

#include <streambuf>
#include <iostream>
#include <sstream>

// About Desktop OpenGL function loaders:
//  Modern desktop OpenGL doesn't have a standard portable header file to load OpenGL function pointers.
//  Helper libraries are often used for this purpose! Here we are supporting a few common ones (gl3w, glew, glad).
//  You may use another loader/header of your choice (glext, glLoadGen, etc.), or chose to manually implement your own.
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
#include <GL/gl3w.h>            // Initialize with gl3wInit()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
#include <GL/glew.h>            // Initialize with glewInit()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
#include <glad/glad.h>          // Initialize with gladLoadGL()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD2)
#include <glad/gl.h>            // Initialize with gladLoadGL(...) or gladLoaderLoadGL()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING2)
#define GLFW_INCLUDE_NONE       // GLFW including OpenGL headers causes ambiguity or multiple definition errors.
#include <glbinding/Binding.h>  // Initialize with glbinding::Binding::initialize()
#include <glbinding/gl/gl.h>
using namespace gl;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING3)
#define GLFW_INCLUDE_NONE       // GLFW including OpenGL headers causes ambiguity or multiple definition errors.
#include <glbinding/glbinding.h>// Initialize with glbinding::initialize()
#include <glbinding/gl/gl.h>
using namespace gl;
#else
#include IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#endif

// Forward declare to keep main func on top for readability
int SDLInit(SDL_Window** window, SDL_GLContext* gl_context, const char** glsl_version);
ImGuiIO& ImGUIInit(SDL_Window* window, SDL_GLContext* gl_context, const char* glsl_version);
void UILoop(SDL_Window* window, ImGuiIO& io );
void Cleanup(SDL_Window* window, SDL_GLContext* gl_context);

void ShowConfigWindow(bool * showLog);
void ShowLogWindow(ImGuiTextBuffer&);
int UpdateActors(float deltaTime, bool * showLog);
bool Load(const char* fileName);
void Clear();

void RegisterActors();

volatile sig_atomic_t stop;
void inthand(int signum) {
    stop = 1;
}

// Window variables
SDL_Window* window;
SDL_GLContext gl_context;
ImGuiIO io;
const char* glsl_version;

// Logging
bool logWindow = false;
char huge_string_buf[4096];
int out_pipe[2];

ImGuiTextBuffer& getBuffer(){
    static ImGuiTextBuffer sLogBuffer; // static log buffer for logger channel

    //read(out_pipe[0], huge_string_buf, 4096);
    if ( strlen( huge_string_buf ) > 0 ) {
        sLogBuffer.appendf("%s", huge_string_buf);
        memset(huge_string_buf,0,4096);
    }

    return sLogBuffer;
}

// Main code
int main(int argc, char** argv)
{
    RegisterActors();

    // Argument capture
    bool headless = false;
    int loops = -1;
    int loopCount = 0;
    for (int i = 0; i < argc; ++i) {
        if (strcmp(argv[i], "-h") == 0 )
        {
            if ( i + 1 < argc ) {
                // try to load the given file
                if ( Load(argv[i+1])) {
                    zsys_info("Loaded file: %s", argv[i+1]);
                    headless = true;
                }
                else {
                    zsys_info("Headless run error. Stage file not found.");
                    return -1;
                }
            }
            else {
                zsys_info("Headless run error. No stage file provided.");
                return -1;
            }
        }
        else if(strcmp(argv[i], "-test") == 0 ) {
            if ( Load("test.txt")) {
                headless = true;
                loops = 10000;
            }
            else {
                zsys_info("Test error. Stage file not found.");
                return -1;
            }
        }
    }

    /*
    if (!headless) {
        //TODO: Fix non-threadsafeness causing hangs on zsys_info calls during zactor_destroy
        // capture stdout
        // This approach uses a pipe to prevent multiple writes before reads overlapping
        memset(huge_string_buf,0,4096);

        int rc = pipe(out_pipe);
        assert( rc == 0 );

        long flags = fcntl(out_pipe[0], F_GETFL);
        flags |= O_NONBLOCK;
        fcntl(out_pipe[0], F_SETFL, flags);

        dup2(out_pipe[1], STDOUT_FILENO);
        close(out_pipe[1]);  
    }
    */

    //TODO: Implement an argument to allow opening a window during a headless run
    if ( headless ) {
        signal(SIGINT, inthand);

        while (!stop) {
            if ( loops != -1 ) {
                std::this_thread::sleep_for (std::chrono::milliseconds(1));
                if ( ++loopCount > loops ) {
                    break;
                }
            }
        }
    }
    else {
        int result = SDLInit(&window, &gl_context, &glsl_version);
        if ( result != 0 ) {
            return result;
        }

        zsys_info("VERSION: %s", glsl_version);
        io = ImGUIInit(window, &gl_context, glsl_version);

        // Blocking UI loop
        UILoop(window, io);

        // Cleanup
        Cleanup(window, &gl_context);
    }

    Clear();
    sphactor_dispose();

    fflush(stdout);
    return 0;
}

int SDLInit( SDL_Window** window, SDL_GLContext* gl_context, const char** glsl_version ) {
    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER | SDL_INIT_AUDIO) != 0)
    {
        zsys_info("Error: %s", SDL_GetError());
        return -1;
    }

    // Decide GL+GLSL versions
#if __APPLE__
    // GL 3.2 Core + GLSL 150
    *glsl_version = new char[13] { "#version 150" };
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    // GL 3.0 + GLSL 130
    *glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

    // Create window with graphics context
    //TODO: Make this a skippable part of the system...
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    *window = SDL_CreateWindow("GazebOSC", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
    *gl_context = SDL_GL_CreateContext(*window);
    SDL_GL_MakeCurrent(*window, *gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Initialize OpenGL loader
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
    bool err = gl3wInit() != 0;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
    bool err = glewInit() != GLEW_OK;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
    bool err = gladLoadGL() == 0;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD2)
    bool err = gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress) == 0; // glad2 recommend using the windowing library lo>
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING2)
    bool err = false;
    glbinding::Binding::initialize();
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING3)
    bool err = false;
    glbinding::initialize([](const char* name) { return (glbinding::ProcAddress)SDL_GL_GetProcAddress(name); });
#else
    bool err = false; // If you use IMGUI_IMPL_OPENGL_LOADER_CUSTOM, your loader is likely to requires some form of in>
#endif
    if (err)
    {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
        return 1;
    }

    return 0;
}

ImGuiIO& ImGUIInit(SDL_Window* window, SDL_GLContext* gl_context, const char* glsl_version) {
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer bindings
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui:>
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multipl>
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (>
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontA>
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double ba>
    //io.Fonts->AddFontDefault();
    ImFontConfig font_cfg = ImFontConfig();
    font_cfg.OversampleH = font_cfg.OversampleV = 1;
    font_cfg.PixelSnapH = true;
    font_cfg.SizePixels = 13.0f * 1.0f;
    font_cfg.EllipsisChar = (ImWchar)0x0085;
    font_cfg.GlyphOffset.y = 1.0f * IM_FLOOR(font_cfg.SizePixels / 13.0f);  // Add +1 offset per 13 units
    io.Fonts->AddFontFromFileTTF("misc/fonts/ProggyCleanSZ.ttf", 13.0f, &font_cfg);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphR>
    //IM_ASSERT(font != NULL);

    return io;
}

void UILoop( SDL_Window* window, ImGuiIO& io ) {
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
    bool done = false;
    unsigned int deltaTime = 0, oldTime = 0;
    while (!done)
    {
        static int w, h;
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame(window);
        ImGui::NewFrame();

        //main window
        //  this will be our main workspace

        SDL_GetWindowSize(window, &w, &h);
        ImVec2 size = ImVec2(w,h);
        ImGui::SetNextWindowSize(size);

        // Get time since last frame
        deltaTime = SDL_GetTicks() - oldTime;
        /* In here we can poll sockets */
        if (deltaTime < 1000/30)
            SDL_Delay((1000/30)-deltaTime);
        //printf("fps %.2f %i\n", 1000./(SDL_GetTicks() - oldTime), deltaTime);
        /* For when we send msgs to the main thread */
        oldTime = SDL_GetTicks();
        int rc = UpdateActors( ((float)deltaTime) / 1000, &logWindow);

        if ( rc == -1 ) {
            done = true;
        }

        // Save/load window
        //size = ImVec2(350,135);
        //ImVec2 pos = ImVec2(w - 400, 50);
        //ImGui::SetNextWindowSize(size);
        //ImGui::SetNextWindowPos(pos);
        //ShowConfigWindow(&logWindow);

        if ( logWindow ) {
            ShowLogWindow(getBuffer());
        }

        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }
}

void Cleanup( SDL_Window* window, SDL_GLContext* gl_context) {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(*gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
