#ifndef WINDOW_HPP
#define WINDOW_HPP
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#   define IMGUI_DEFINE_MATH_OPERATORS
#   define IM_VEC2_CLASS_EXTRA
#endif
#include "StageWindow.hpp"
#include "TextEditorWindow.hpp"
#include "AboutWindow.hpp"
#include "LogWindow.hpp"
#include "DemoWindow.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include <string>
#include "fontawesome5.h"
#include "config.h"
#include "libsphactor.h"

namespace gzb {

struct App
{
    ImGuiTextBuffer log_buffer;
    std::vector<TextEditorWindow*> text_editors;
    AboutWindow about_win;
    LogWindow log_win;
    DemoWindow demo_win;
    StageWindow stage_win;

    static App& getApp() {
        static App app;
        return app;
    }
    App(): log_win(&log_buffer) {};
    ~App() {
        for (auto w : text_editors)
            delete w;
    };
};
static void capture_stdio(int pipe_in, int pipe_out)
{
#ifdef __WINDOWS__
    printf("WARNING: stdio redirection not implemented on Windows\n");
#else
    //TODO: Fix non-threadsafeness causing hangs on zsys_info calls during zactor_destroy
    long flags = fcntl(pipe_in, F_GETFL);
    flags |= O_NONBLOCK;
    fcntl(pipe_in, F_SETFL, flags);

    dup2(pipe_out, STDOUT_FILENO);
    close(pipe_out);
#endif
}

}
#endif // WINDOW_HPP
