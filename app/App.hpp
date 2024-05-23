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
#include "PyWindow.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include <string>
#include "fontawesome5.h"
#include "config.h"
#include "libsphactor.h"

namespace gzb {

struct App
{
    // settings
    unsigned int fps = 60;
    unsigned int idle_fps = 10;
    // windows
    ImGuiTextBuffer log_buffer;
    std::vector<TextEditorWindow*> text_editors;
    AboutWindow about_win;
    LogWindow log_win;
    DemoWindow demo_win;
    StageWindow stage_win;
#ifdef PYTHON3_FOUND
    PyWindow py_win;
#endif
    static App& getApp() {
        static App app;
        return app;
    }
    App(): log_win(&log_buffer) {};
    ~App() {
        for (auto w : text_editors)
            delete w;
    };

    int Update() {
        // root window
        int rc = stage_win.UpdateActors();
        if ( rc == -1)
            return rc;

        // text editor windows
        std::vector<gzb::TextEditorWindow *>::iterator itr = text_editors.begin();
        while ( itr < text_editors.end() )
        {
            if ( (*itr)->showing )
                (*itr)->OnImGui();
            if ( (*itr)->requesting_destroy )
            {
                delete(*itr);
                itr = text_editors.erase(itr);
            }
            else
                ++itr;
        }
        if (about_win.showing)
            about_win.OnImGui();
        if (log_win.showing)
            log_win.OnImGui();
        if (demo_win.showing)
            demo_win.OnImGui();
        if (py_win.showing)
            py_win.OnImGui();

        return rc;
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
