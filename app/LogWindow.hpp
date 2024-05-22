#ifndef LOGWINDOW_HPP
#define LOGWINDOW_HPP

#include "app/Window.hpp"
#include "imgui.h"
#include <fcntl.h>
#ifdef __unix__
#include <unistd.h>
#endif
namespace gzb {
struct LogWindow : public Window
{
    bool scroll_to_bottom = true;
    ImGuiTextBuffer *buffer;
    bool capture_stdout = false;
    int stdout_pipe[2];
    int original_stdout_fd = -1;

    LogWindow(ImGuiTextBuffer *log_buffer)
    {
        buffer = log_buffer;
        window_name = "Log Console";
    };
    ~LogWindow() {};
    void OnImGui()
    {
        ReadStdOut();
        ImGui::PushID(123);

        ImGui::SetNextWindowSizeConstraints(ImVec2(100,100), ImVec2(1000,1000));
        ImGui::Begin(window_name.c_str(), &showing);

        if (ImGui::Button("Clear")) buffer->clear();
        ImGui::SameLine();
        if ( ImGui::Button("To Bottom") ) {
            scroll_to_bottom = true;
        }

        ImGui::BeginChild("scrolling", ImVec2(0,0), false, ImGuiWindowFlags_HorizontalScrollbar);

        if ( !scroll_to_bottom && ImGui::GetScrollY() == ImGui::GetScrollMaxY() ) {
            scroll_to_bottom = true;
        }

        ImGui::TextUnformatted(buffer->begin());

        if (scroll_to_bottom)
            ImGui::SetScrollHereY(1.0f);
        scroll_to_bottom = false;

        ImGui::EndChild();

        ImGui::End();

        ImGui::PopID();
    }

    int ReadStdOut()
    {
        int rc = 0;
#ifdef __WINDOWS__
#else
        static char huge_string_buf[4096];

        if (capture_stdout)
            rc = read(stdout_pipe[0], huge_string_buf, 4096);
        if ( rc > 0 && strlen( huge_string_buf ) > 0 ) {
            buffer->appendf("%s", huge_string_buf);
            memset(huge_string_buf,0,4096);
        }
#endif
        return rc;
    }

    int CaptureStdOut()
    {
#ifdef __WINDOWS__
        capture_stdout = false;
        return -1;
        printf("WARNING: stdio redirection not implemented on Windows\n");
        /*
     * https://jeffpar.github.io/kbarchive/kb/058/Q58667/
    FILE *stream ;
    if((stream = freopen("file.txt", "w", stdout)) == NULL)
        exit(-1);
    printf("this is stdout output\n");
    stream = freopen("CON", "w", stdout);
    printf("And now back to the console once again\n");
    */
#else
        //TODO: Fix non-threadsafeness causing hangs on zsys_info calls during zactor_destroy
        fflush(stdout);
        //fpos_t pos;
        //fgetpos(stdout, &pos);
        // create new pipe
        if (pipe(stdout_pipe) == -1) {
            perror("pipe");
            return -1;
        }
        original_stdout_fd = dup(STDOUT_FILENO);

        int fd = dup2(stdout_pipe[1], STDOUT_FILENO);
        assert(fd);
        close(stdout_pipe[1]);

        int flags = fcntl(stdout_pipe[0], F_GETFL);
        if (flags == -1) {
            perror("fcntl");
            return -1;
        }
        if (fcntl(stdout_pipe[0], F_SETFL, flags | O_NONBLOCK) == -1) {
            perror("fcntl");
            return -1;
        }

        capture_stdout = true;
        return fd;
#endif
    }

    int RevertStdOut()
    {
#ifdef __WINDOWS__
        printf("WARNING: stdio redirection not implemented on Windows\n");
        return -1;
#else
        int rc = 0;
        if (original_stdout_fd != -1)
        {
            rc = dup2(original_stdout_fd, STDOUT_FILENO);
            close(original_stdout_fd);
            original_stdout_fd = -1;
        }
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        capture_stdout = false;
        return rc;
#endif
    }
};
} // namespace
#endif // LOGWINDOW_HPP
