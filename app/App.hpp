#ifndef WINDOW_HPP
#define WINDOW_HPP
#include "TextEditorWindow.hpp"
#include <string>

namespace gzb {
struct App
{
    static App& getApp() {
        static App app;
        return app;
    }
    App() {};

    ImGuiTextBuffer log_buffer;
    std::vector<gzb::TextEditorWindow*> text_editors;
};
}
#endif // WINDOW_HPP
