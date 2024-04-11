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
#include "ext/ImGui-Addons/FileBrowser/ImGuiFileBrowser.h"
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

    int OnImGuiMenuBar()
    {
        static char* configFile = new char[64] { 0x0 };
        static int keyFocus = 0;
        MenuAction action = MenuAction_None;

        if ( keyFocus > 0 )
            keyFocus--;

        ImGui::BeginMainMenuBar();

        if ( ImGui::BeginMenu("File") ) {
            if ( ImGui::MenuItem(ICON_FA_FOLDER_OPEN " Load") ) {
                //TODO: support checking if changes were made
                action = MenuAction_Load;
            }
            if ( ImGui::MenuItem(ICON_FA_SAVE " Save") ) {
                action = MenuAction_Save;
            }
            if ( ImGui::MenuItem(ICON_FA_SAVE " Save As") ) {
                action = MenuAction_SaveAs;
            }
            ImGui::Separator();
            ImGui::Separator();
            if ( ImGui::MenuItem(ICON_FA_WINDOW_CLOSE " Exit") ) {
                //TODO: support checking if changes were made
                action = MenuAction_Exit;
            }

            ImGui::EndMenu();
        }

        if ( ImGui::BeginMenu("Stage") ) {
            if ( ImGui::MenuItem(ICON_FA_TRASH_ALT " Clear") ) {
                action = MenuAction_Clear;
            }
            ImGui::EndMenu();
        }

        if ( ImGui::BeginMenu("Tools") ) {
            if ( ImGui::BeginMenu(ICON_FA_EDIT " Text Editor") )
            {
                if ( ImGui::MenuItem(ICON_FA_FOLDER_OPEN " New editor") )
                {
                    gzb::App::getApp().text_editors.push_back(new gzb::TextEditorWindow());
                }
                for (auto w : gzb::App::getApp().text_editors )
                {
                    std::string title = w->window_name + " " + ICON_FA_EYE;
                    if ( ImGui::MenuItem(w->window_name.c_str(), NULL, w->showing, true ) )
                        w->showing = !w->showing;
                }
                ImGui::EndMenu();
            }
            if ( ImGui::MenuItem(ICON_FA_TERMINAL " Toggle Log Console") ) {
                gzb::App::getApp().log_win.showing = !gzb::App::getApp().log_win.showing;
            }
#ifdef HAVE_IMGUI_DEMO
            if ( ImGui::MenuItem(ICON_FA_CARAVAN " Toggle Demo") ) {
                gzb::App::getApp().demo_win.showing = !gzb::App::getApp().demo_win.showing;
            }
#endif
            if ( ImGui::MenuItem(ICON_FA_INFO " Toggle About") ) {
                gzb::App::getApp().about_win.showing = !gzb::App::getApp().about_win.showing;
            }

            ImGui::EndMenu();
        }

        //TODO: Display stage status (new, loaded, changed)
        ImGui::Separator();
        char path[PATH_MAX];
        getcwd(path, PATH_MAX);

        ImGui::Separator();
        ImGui::TextColored( ImVec4(.3,.5,.3,1), ICON_FA_FOLDER ": %s", path);

        if ( ImGui::IsItemHovered() )
        {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 24.0f);
            ImGui::TextUnformatted("Current working directory, double click to open");
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();

            if ( ImGui::IsMouseDoubleClicked(0) )
            {
                char cmd[PATH_MAX];
#ifdef __WINDOWS__
                snprintf(cmd, PATH_MAX, "start explorer %s", path);
                //snprintf(cmd, PATH_MAX, "explorer.exe %s", path);
                //STARTUPINFO startupInfo = {0};
                //startupInfo.cb = sizeof(startupInfo);
                //PROCESS_INFORMATION processInformation;
                //CreateProcess(NULL, cmd, NULL, NULL, false, 0, NULL, NULL, &startupInfo, &processInformation);
#elif defined __UTYPE_LINUX
                snprintf(cmd, PATH_MAX, "xdg-open %s &", path);
#else
                snprintf(cmd, PATH_MAX, "open %s", path);
#endif
                system(cmd);
            }
        }

        ImGui::Separator();


        if ( streq( stage_win.editing_file.c_str(), "" ) ) {
            ImGui::TextColored( ImVec4(.7,.9,.7,1), ICON_FA_EDIT ": *Unsaved Stage*");
        }
        else {
            ImGui::TextColored( ImVec4(.7,.9,.7,1), ICON_FA_EDIT ": %s", stage_win.editing_file.c_str());
        }

        if (GZB_GLOBAL.UPDATE_AVAIL)
        {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - 64);
            ImGui::TextColored( ImVec4(.99,.9,.7,1), "update " ICON_FA_INFO_CIRCLE );
            if ( ImGui::IsItemHovered() )
            {
                ImGui::BeginTooltip();
                ImGui::PushTextWrapPos(ImGui::GetFontSize() * 24.0f);
                ImGui::TextUnformatted("Gazebosc update available, click to download");
                ImGui::PopTextWrapPos();
                ImGui::EndTooltip();

                if ( ImGui::IsMouseClicked(0) )
                {
                    char cmd[PATH_MAX];
                    char url[60] = "https://pong.hku.nl/~buildbot/gazebosc/";
#ifdef __WINDOWS__
                    snprintf(cmd, PATH_MAX, "start %s", url);
#elif defined __UTYPE_LINUX
                    snprintf(cmd, PATH_MAX, "xdg-open %s &", url);
#else
                    snprintf(cmd, PATH_MAX, "open %s", url);
#endif
                    system(cmd);
                }
            }
        }

        ImGui::EndMainMenuBar();

        // Handle MenuActions
        if ( action == MenuAction_Load) {
            ImGui::OpenPopup("MenuAction_Load");
            keyFocus = 2;
        }
        else if ( action == MenuAction_Save ) {
            if ( streq( stage_win.editing_file.c_str(), "" ) ) {
                ImGui::OpenPopup("MenuAction_Save");
            }
            else {
                stage_win.Save(stage_win.editing_path.c_str());
                ImGui::CloseCurrentPopup();
            }
        }
        else if ( action == MenuAction_SaveAs ) {
            ImGui::OpenPopup("MenuAction_Save");
        }
        else if ( action == MenuAction_Clear ) {
            stage_win.Clear();
            stage_win.Init();
        }
        else if ( action == MenuAction_Exit ) {
            return -1;
        }

        /*if(file_dialog.showFileDialog("MenuAction_Load", imgui_addons::ImGuiFileBrowser::DialogMode::OPEN, ImVec2(700, 310), ".gzs"))
        {
            if (Load(file_dialog.selected_path.c_str())) {
                editingFile = file_dialog.selected_fn;
                editingPath = file_dialog.selected_path;

                moveCwdIfNeeded();
            }
        }

        if(file_dialog.showFileDialog("MenuAction_Save", imgui_addons::ImGuiFileBrowser::DialogMode::SAVE, ImVec2(700, 310), ".gzs"))
        {
            if ( !Save(file_dialog.selected_path.c_str()) ) {
                editingFile = "";
                editingPath = "";
                zsys_error("Saving file failed!");
            }
            else {
                editingFile = file_dialog.selected_fn;
                editingPath = file_dialog.selected_path;

                moveCwdIfNeeded();
            }
        }*/

        return 0;
    }
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
