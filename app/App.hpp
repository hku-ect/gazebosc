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

        return rc;
    };

};

}
#endif // WINDOW_HPP
