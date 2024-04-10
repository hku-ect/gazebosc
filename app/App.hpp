#ifndef WINDOW_HPP
#define WINDOW_HPP
#include "TextEditorWindow.hpp"
#include "AboutWindow.hpp"
#include "LogWindow.hpp"
#include "DemoWindow.hpp"
#include <string>

namespace gzb {
struct App
{
    static App& getApp() {
        static App app;
        return app;
    }
    App(): log_win(&log_buffer) {};
    ~App() {
        for (auto w : text_editors)
            delete w;
    };

    ImGuiTextBuffer log_buffer;
    std::vector<TextEditorWindow*> text_editors;
    AboutWindow about_win;
    LogWindow log_win;
    DemoWindow demo_win;
};

static ImGuiTextBuffer& getLogBuffer(int fd=-1){
    static ImGuiTextBuffer sLogBuffer; // static log buffer for logger channel
    static char huge_string_buf[4096];

    if (fd !=-1)
        read(fd, huge_string_buf, 4096);
    if ( strlen( huge_string_buf ) > 0 ) {
        sLogBuffer.appendf("%s", huge_string_buf);
        memset(huge_string_buf,0,4096);
    }

    return sLogBuffer;
}

static void capture_stdio()
{
    static int out_pipe[2];
    int rc = pipe(out_pipe);
    assert( rc == 0 );

    long flags = fcntl(out_pipe[0], F_GETFL);
    flags |= O_NONBLOCK;
    fcntl(out_pipe[0], F_SETFL, flags);

    dup2(out_pipe[1], STDOUT_FILENO);
    close(out_pipe[1]);
}

}
#endif // WINDOW_HPP
