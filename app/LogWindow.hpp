#ifndef LOGWINDOW_HPP
#define LOGWINDOW_HPP

#include "app/Window.hpp"
#include "imgui.h"
#include <unistd.h>

namespace gzb {
struct LogWindow : public Window
{
    int pipe_fd = -1;
    bool scroll_to_bottom = true;
    ImGuiTextBuffer *buffer;

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
        ImGui::Begin(window_name.c_str());

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
        static char huge_string_buf[4096];
        int rc = 0;

        if (pipe_fd !=-1)
            rc = read(pipe_fd, huge_string_buf, 4096);
        if ( rc > 0 && strlen( huge_string_buf ) > 0 ) {
            buffer->appendf("%s", huge_string_buf);
            memset(huge_string_buf,0,4096);
        }
        return rc;
    }
};
} // namespace
#endif // LOGWINDOW_HPP
