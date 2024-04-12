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

#include "libsphactor.h"
#ifdef __UTYPE_OSX
// needed to change the wd to Resources of the app bundle
#include "CoreFoundation/CoreFoundation.h"
#elif defined(__WINDOWS__)
#include<dbghelp.h>
#include"StackWalker.h"
#endif
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#   define IMGUI_DEFINE_MATH_OPERATORS
#   define IM_VEC2_CLASS_EXTRA
#endif
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include "imgui_internal.h"
#include "fontawesome5.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL_opengles2.h>
#else
#include <SDL_opengl.h>
#endif

#include <chrono>
#include <thread>
#include <stdio.h>
#include <signal.h>
#include <streambuf>
#include <iostream>
#include <sstream>
#include <filesystem>
namespace fs = std::filesystem;

#include "config.h"
#include "ext/ImFileDialog/ImFileDialog.h"
#include "actors/actors.h"
#include "app/App.hpp"

// Forward declare to keep main func on top for readability
int SDLInit(SDL_Window** window, SDL_GLContext* gl_context, const char** glsl_version);
ImGuiIO& ImGUIInit(SDL_Window* window, SDL_GLContext* gl_context, const char* glsl_version);
void UILoop(SDL_Window* window, ImGuiIO& io );
void Cleanup(SDL_Window* window, SDL_GLContext* gl_context);
void register_actors();
// Window variables
SDL_Window* window;
SDL_GLContext gl_context;
ImGuiIO io;
const char* glsl_version;
static ImWchar ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
GZB_GLOBALS_t GZB_GLOBAL;
// Logging
char huge_string_buf[4096];
int out_pipe[2];

// exit handlers et al
volatile sig_atomic_t stop;

void handle_exit(void)
{
    stop = 1;
}

void sig_hand(int signum) {
    handle_exit();
}

#ifdef __WINDOWS__
static BOOL WINAPI s_exit_handler_fn (DWORD ctrltype)
{
    handle_exit();
    return TRUE;
}
#endif

// Obtain a backtrace and print it to stdout.
#if defined(HAVE_LIBUNWIND)
#define UNW_LOCAL_ONLY
#include <libunwind.h>
#include <stdlib.h>

static int tracecount = 0;
void
print_backtrace (int sig)
{
    unw_cursor_t cursor;
    unw_context_t context;

    // grab the machine context and initialize the cursor
    if (unw_getcontext(&context) < 0)
        zsys_error("ERROR: cannot get local machine state\n");
    if (unw_init_local(&cursor, &context) < 0)
        zsys_error("ERROR: cannot initialize cursor for local unwinding\n");


    // currently the IP is within backtrace() itself so this loop
    // deliberately skips the first frame.
    while (unw_step(&cursor) > 0 && tracecount < 16) {
        unw_word_t offset, pc;
        char sym[4096];
        if (unw_get_reg(&cursor, UNW_REG_IP, &pc))
            zsys_error("ERROR: cannot read program counter\n");

        printf("0x%lx: ", pc);

        if (unw_get_proc_name(&cursor, sym, sizeof(sym), &offset) == 0)
            printf("(%s+0x%lx)\n", sym, offset);
        else
            printf("-- no symbol name found\n");
        tracecount++;
    }
    wchar_t tl=0x250f;
    wchar_t tr=0x2513;
    wchar_t bl=0x2517;
    wchar_t br=0x251b;
    wchar_t vs=0x2503;
    wchar_t hs[53]=L"━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━";
    setlocale(LC_ALL, "");
    fwide(stdout, 1);
    printf("Gazebo version %s\n", GIT_VERSION);
    printf("\033[5m\033[31m%lc%ls%lc\n", tl, hs, tr);
    printf("%lc\033[0m\033[31m%s\033[5m%lc\n", vs, "  Software Failure.  Please report above details.   ", vs);
    printf("%lc\033[0m\033[31m%s%08X%s\033[5m%lc\n", vs, "            Guru Meditation #", sig, "               ", vs);
    printf("%lc%ls%lc\n", bl, hs, br);
    printf("\033[39m\033[0m");
    fflush(stdout);
    // TODO try proper exit to close sockets
    zsys_shutdown();
    exit(13);
}
#elif defined __WINDOWS__
class MyStackWalker : public StackWalker
{
public:
    MyStackWalker() : StackWalker(StackWalker::RetrieveLine, 
                                    NULL, GetCurrentProcessId(),
                                    GetCurrentProcess()) {}
protected:
    virtual void OnOutput(LPCSTR szText) {
        printf(szText); StackWalker::OnOutput(szText);
    }
};

void print_backtrace(int sig);
void print_backtrace(int sig)
{
    MyStackWalker sw; 
    sw.ShowCallstack();
    // enable ANSI codes
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    // Clear the quick edit bit in the mode flags (this will hang the UI)
    dwMode &= ~ENABLE_QUICK_EDIT_MODE;
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
    //SetConsoleOutputCP(437);
    setlocale(LC_ALL, "");
    _setmode(_fileno(stdout), _O_U16TEXT);
    wchar_t tl = L'┏';
    wchar_t tr = L'┓';
    wchar_t bl = L'┗';
    wchar_t br = L'┛';
    wchar_t vs = L'┃';
    wchar_t hs[53] = L"━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━";
    // Windows does not support blinking :S
    wprintf(L"Gazebo version %s\n", GIT_VERSION);
    wprintf(L"\033[5m\033[31m%lc%ls%lc\n", tl, hs, tr);
    wprintf(L"%lc\033[0m\033[31m%s\033[5m%lc\n", vs, L"  Software Failure.  Press enter key to continue.   ", vs);
    wprintf(L"%lc\033[0m\033[31m%s%08X%s\033[5m%lc\n", vs, L"            Guru Meditation #", sig, L"               ", vs);
    wprintf(L"%lc%ls%lc\n", bl, hs, br);
    wprintf(L"\033[39m\033[0m");
    std::cin.get(); // hack to stop the console from closing
}
#else
void print_backtrace (int sig) {}
#endif

// This sets the global RESOURCESPATH var to the path of the executable or the GZB_RESOURCES environment value
void set_global_resources()
{
    char *path = getenv("GZB_RESOURCES");
    if (path)
    {
        GZB_GLOBAL.RESOURCESPATH = path;
        return;
    }
#ifdef  __WINDOWS__
    wchar_t exepath[PATH_MAX];
    GetModuleFileNameW(NULL, exepath, PATH_MAX);
    // remove program name by finding the last delimiter
    wchar_t *s = wcsrchr(exepath, L'\\');
    if (s) *s = 0;
    size_t i;
    char winpath[PATH_MAX];
    wcstombs_s(&i, winpath, (size_t)PATH_MAX,
                   exepath, (size_t)PATH_MAX - 1); // -1 so the appended NULL doesn't fall outside the allocated buffer
    GZB_GLOBAL.RESOURCESPATH = strdup(winpath);
    zsys_info("Resources dir is %s", GZB_GLOBAL.RESOURCESPATH);
#elif defined __UTYPE_LINUX
    char exepath[PATH_MAX];
    memset(exepath, 0, PATH_MAX);
    readlink("/proc/self/exe", exepath, PATH_MAX);
    char *lastdelim = strrchr(exepath, '/');
    assert(lastdelim);
    exepath[lastdelim-exepath] = 0;
    GZB_GLOBAL.RESOURCESPATH = strdup(exepath);
    zsys_info("Resources dir is %s", GZB_GLOBAL.RESOURCESPATH);
#elif defined __UTYPE_OSX
    char osxpath[PATH_MAX];
    CFURLRef res = CFBundleCopyResourcesDirectoryURL(CFBundleGetMainBundle());
    CFURLGetFileSystemRepresentation(res, TRUE, (UInt8 *)osxpath, PATH_MAX);
    CFRelease(res);
    GZB_GLOBAL.RESOURCESPATH = strdup(osxpath);
    zsys_info("Resources dir is %s", GZB_GLOBAL.RESOURCESPATH);
#endif
}

// This sets the global RESOURCESPATH var to the path of the executable or the GZB_RESOURCES environment value
void set_global_temp()
{
    char *path = getenv("GZB_TMP");
    if (path)
    {
        GZB_GLOBAL.TMPPATH = strdup(path);
        zsys_info("Tmp dir set from gazebosc env var: %s", GZB_GLOBAL.TMPPATH);
        return;
    }
    fs::path tmppath = fs::canonical(fs::temp_directory_path());
    GZB_GLOBAL.TMPPATH = strdup(tmppath.string().c_str());
    /*
#ifdef  __WINDOWS__
    char *envpath = getenv("TEMP");
    if (envpath)
        GZB_GLOBAL.TMPPATH = strdup(envpath);
    else
    {
        wchar_t exepath[PATH_MAX];
        GetModuleFileNameW(NULL, exepath, PATH_MAX);
        // remove program name by finding the last delimiter
        wchar_t *s = wcsrchr(exepath, L'\\');
        if (s) *s = 0;
        size_t i;
        char winpath[PATH_MAX];
        wcstombs_s(&i, winpath, (size_t)PATH_MAX,
                       exepath, (size_t)PATH_MAX - 1); // -1 so the appended NULL doesn't fall outside the allocated buffer
        GZB_GLOBAL.RESOURCESPATH = strdup(winpath);
    }
#elif defined __UTYPE_LINUX
    char *tmppath = getenv("TMPDIR");
    if (tmppath)
    {
        GZB_GLOBAL.TMPPATH = strdup(tmppath);
    }
    else if ( zsys_file_exists("/var/tmp") )
    {
        GZB_GLOBAL.TMPPATH = strdup("/var/tmp");
    }
    else
        GZB_GLOBAL.TMPPATH = strdup("/tmp");
#elif defined __UTYPE_OSX
    char tmppath[PATH_MAX];
    size_t n = confstr(_CS_DARWIN_USER_TEMP_DIR, tmppath, sizeof(tmppath));
    if ((n <= 0) || (n >= sizeof(tmppath)))
        strlcpy(tmppath, getenv("TMPDIR"), sizeof(tmppath));
    GZB_GLOBAL.TMPPATH = strdup(tmppath);

    //CFURLRef tmp = CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault, (UInt8 *)tmpdir, strlen(tmpdir), true);

    //NSString *tempDir = NSTemporaryDirectory();
    //if (tempDir == nil)
    //    tempDir = @"/tmp";
    //GZB_GLOBAL.TMPPATH = strdup( (char *)[tempDir UTF8String]);
#endif
    */
    zsys_info("Tmp dir is %s", GZB_GLOBAL.TMPPATH);
}

// Main code
int main(int argc, char** argv)
{
    zsys_init();          //  when calling exit() a warning will be issued about dangling sockets, this
    atexit(handle_exit);  //  is a false positive as our exit handler will clean them up as well afterwards
    signal(SIGINT, sig_hand);
    signal(SIGSEGV, print_backtrace); // Invalid memory reference
    signal(SIGABRT, print_backtrace); // Abort signal from abort(3)
    signal(SIGFPE,  print_backtrace); // Floating-point exception
#if defined(__UNIX__)
    signal(SIGQUIT, sig_hand);
    signal(SIGTERM, sig_hand);
    signal(SIGHUP, sig_hand);
    signal(SIGILL, print_backtrace); // Illegal Instruction
    signal(SIGBUS, print_backtrace); // Bus error (bad memory access)
    signal(SIGSYS,  print_backtrace); // Bad system call (SVr4);
    signal(SIGXCPU, print_backtrace); // CPU time limit exceeded (4.2BSD)
#ifdef __UTYPE_OSX
// not available anymore?
//    signal(SIGFSZ,  print_backtrace); // File size limit exceeded (4.2BSD)
    char path[PATH_MAX];
    CFURLRef res = CFBundleCopyResourcesDirectoryURL(CFBundleGetMainBundle());
    CFURLGetFileSystemRepresentation(res, TRUE, (UInt8 *)path, PATH_MAX);
    CFRelease(res);
    chdir(path);
    zsys_info("working dir changed to %s", path);
#endif
#elif defined(__WINDOWS__)
    SetConsoleCtrlHandler (s_exit_handler_fn, TRUE);
#endif

    //
    set_global_resources();
    set_global_temp();
    GZB_GLOBAL.UPDATE_AVAIL = false;
    register_actors();

    // Argument capture
    zargs_t *args = zargs_new(argc, argv);
    if ( zargs_hasx (args, "--help", "-h", NULL) )
    {
        zargs_print(args);
        return 0;
    }
    bool verbose = zargs_hasx (args, "--verbose", "-v", NULL);
    bool headless = zargs_hasx (args, "--background", "-b", NULL);
    const char *stage_file = zargs_first(args);

    stop = 0;
    int loops = -1;
    int loopCount = 0;

    if (!headless)
    {
        memset(huge_string_buf,0,4096);
        int rc = pipe(out_pipe);
        assert( rc == 0 );
        gzb::capture_stdio(out_pipe[0], out_pipe[1]);
    }

    // try to init SDL and otherwise run headless
    if ( !headless && SDLInit(&window, &gl_context, &glsl_version) == 0 )
    {
        SDL_SetWindowTitle(window, "Gazebosc       [" GIT_VERSION "]" );
        zsys_info("GLSL VERSION: %s", glsl_version);
        io = ImGUIInit(window, &gl_context, glsl_version);

        if ( stage_file )
        {
            if ( ! gzb::App::getApp().stage_win.Load(stage_file))
            {
                zsys_error("Failed loading %s", stage_file);
            }
        }
        else
            gzb::App::getApp().stage_win.Init(); // start with an empty stage

        // Blocking UI loop
        UILoop(window, io);

        // Cleanup
        Cleanup(window, &gl_context);
    }
    //TODO: Implement an argument to allow opening a window during a headless run
    else {

        if ( stage_file )
        {
            if ( ! gzb::App::getApp().stage_win.Load(stage_file))
            {
                zsys_error("Failed loading %s", stage_file);
            }
            while (!stop)
            {
                if ( loops != -1 )
                {
                    std::this_thread::sleep_for (std::chrono::milliseconds(1));
                    if ( ++loopCount > loops )
                        break;
                }
            }
        }
        else if (headless)
        {
            zsys_error("No stage file provided, exiting");
        }
        else
        {
            zsys_error("Can't open a window and no stage file provided, exiting");
        }
    }

    gzb::App::getApp().stage_win.Clear();
    sphactor_dispose();

    zstr_free(&GZB_GLOBAL.RESOURCESPATH);
    zstr_free(&GZB_GLOBAL.TMPPATH);
    zargs_destroy(&args);

    fflush(stdout);
    return 0;
}

ImVector<char*> actor_types;
void UpdateRegisteredActorsCache() {
    zhash_t *hash = sphactor_get_registered();
    zlist_t *list = zhash_keys(hash);
    actor_types.clear();

    char* item = (char*)zlist_first(list);
    while( item ) {
        actor_types.push_back(item);
        item = (char*)zlist_next(list);
    }
}

std::map<std::string, int> max_actors_by_type;
void register_actors() {
    // register stock actors
    sph_stock_register_all();

    sphactor_register("HTTPLaunchpod", &httplaunchpodactor_handler, zconfig_str_load(httplaunchpodactorcapabilities), &httplaunchpodactor_new_helper, NULL); // https://stackoverflow.com/questions/65957511/typedef-for-a-registering-a-constructor-function-in-c
    sphactor_register<OSCOutput>( "OSC Output", OSCOutput::capabilities);
    sphactor_register<OSCMultiOut>( "OSC Multi Output", OSCMultiOut::capabilities);
    sphactor_register<NatNet>( "NatNet", NatNet::capabilities );
    sphactor_register<NatNet2OSC>( "NatNet2OSC", NatNet2OSC::capabilities );
    sphactor_register<Midi2OSC>( "Midi2OSC", Midi2OSC::capabilities );
#ifdef HAVE_OPENVR
    sphactor_register<OpenVR>("OpenVR", OpenVR::capabilities);
#endif
    sphactor_register<OSCInput>( "OSC Input", OSCInput::capabilities );
    sphactor_register<Record>("Record", Record::capabilities );
    sphactor_register<ModPlayerActor>( "ModPlayer", ModPlayerActor::capabilities );
    sphactor_register<ProcessActor>( "Process", ProcessActor::capabilities );
#ifdef HAVE_OPENVR
    sphactor_register<DmxActor>( "DmxOut", DmxActor::capabilities );
    sphactor_register<IntSlider>( "IntSlider", IntSlider::capabilities );
    sphactor_register<FloatSlider>( "FloatSlider", FloatSlider::capabilities );
#endif
#ifdef PYTHON3_FOUND
    int rc = python_init();
    assert( rc == 0);
    /* Check newer version: We should make this async as it slows startup" */
    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();

    PyObject *pUpdateBool = python_call_file_func("checkver", "check_github_newer_commit", "(s)", GIT_HASH);
    if (pUpdateBool && PyObject_IsTrue(pUpdateBool))
        GZB_GLOBAL.UPDATE_AVAIL = true;

    PyGILState_Release(gstate);
    /* End check newer version */
#endif
    //enforcable maximum actor counts
    max_actors_by_type.insert(std::make_pair("NatNet", 1));
    max_actors_by_type.insert(std::make_pair("OpenVR", 1));

    UpdateRegisteredActorsCache();
}

int SDLInit( SDL_Window** window, SDL_GLContext* gl_context, const char** glsl_version ) {
    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER | SDL_INIT_AUDIO) != 0)
    {
        zsys_info("Error: %s", SDL_GetError());
        return -1;
    }

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    *glsl_version = "#version 100";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif __APPLE__
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

    return 0;
}

ImGuiIO& ImGUIInit(SDL_Window* window, SDL_GLContext* gl_context, const char* glsl_version) {
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable;

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
    font_cfg.GlyphOffset.y = 1.0f * IM_TRUNC(font_cfg.SizePixels / 13.0f);  // Add +1 offset per 13 units
    char fontpath[PATH_MAX];
    snprintf(fontpath, PATH_MAX, "%s/%s", GZB_GLOBAL.RESOURCESPATH, "misc/fonts/ProggyCleanSZ.ttf");
    io.Fonts->AddFontFromFileTTF(fontpath, 13.0f, &font_cfg);

    // Add character ranges and merge into main font
    ImFontConfig config;
    config.MergeMode = true;
    config.GlyphMinAdvanceX = 13.0f; // Use if you want to make the icon monospaced
    config.GlyphOffset.y = 1.0f;     // slightly lower than the main font
    snprintf(fontpath, PATH_MAX, "%s/%s", GZB_GLOBAL.RESOURCESPATH, "misc/fonts/fa-solid-900.ttf");
    io.Fonts->AddFontFromFileTTF(fontpath, 13.0f, &config, ranges);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphR>
    //IM_ASSERT(font != NULL);

    // ImFileDialog requires you to set the CreateTexture and DeleteTexture
    ifd::FileDialog::Instance().CreateTexture = [](uint8_t* data, int w, int h, char fmt) -> void* {
        GLuint tex;

        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, (fmt == 0) ? GL_BGRA : GL_RGBA, GL_UNSIGNED_BYTE, data);
        glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE); //Gl2
        //glGenerateMipmap(GL_TEXTURE_2D); //GL3
        glBindTexture(GL_TEXTURE_2D, 0);

        return (void*)tex;
    };
    ifd::FileDialog::Instance().DeleteTexture = [](void* tex) {
        GLuint texID = (GLuint)((uintptr_t)tex);
        glDeleteTextures(1, &texID);
    };

    return io;
}

void UILoop( SDL_Window* window, ImGuiIO& io ) {
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    gzb::App &app = gzb::App::getApp();
    app.log_win.pipe_fd = out_pipe[0];
    // Main loop
    unsigned int deltaTime = 0, oldTime = 0;
    while (!stop)
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
                stop = 1;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                stop = 1;
        }
        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // Get time since last frame
        deltaTime = SDL_GetTicks() - oldTime;
        // In here we can poll sockets
        // For when we send msgs to the main thread
        if (deltaTime < 1000/30)
            SDL_Delay((1000/30)-deltaTime);
        //printf("fps %.2f %i\n", 1000./(SDL_GetTicks() - oldTime), deltaTime);
        oldTime = SDL_GetTicks();

        //main window
        //  this will be our main workspace
        SDL_GetWindowSize(window, &w, &h);
        ImVec2 size = ImVec2(w,h);
        ImGui::SetNextWindowSize(size);

        // root window
        int rc = app.stage_win.UpdateActors(deltaTime);

        // text editor windows
        std::vector<gzb::TextEditorWindow *>::iterator itr = app.text_editors.begin();
        while ( itr < app.text_editors.end() )
        {
            if ( (*itr)->showing )
                (*itr)->OnImGui();
            if ( (*itr)->requesting_destroy )
            {
                delete(*itr);
                itr = app.text_editors.erase(itr);
            }
            else
                ++itr;
        }
        if (app.about_win.showing)
            app.about_win.OnImGui();
        if (app.log_win.showing)
            app.log_win.OnImGui();
        if (app.demo_win.showing)
            app.demo_win.OnImGui();


        if ( rc == -1 ) {
            stop = 1;
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
