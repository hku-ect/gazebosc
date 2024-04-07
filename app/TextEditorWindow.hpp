#ifndef GZBTEXTEDITORWINDOW_HPP
#define GZBTEXTEDITORWINDOW_HPP

#include "Window.hpp"
#include "../ext/ImGuiColorTextEdit/TextEditor.h"

#define FIND_POPUP_TEXT_FIELD_LENGTH 128

namespace gzb
{

struct TextEditorWindow : public Window
{
    TextEditorWindow(const char* filePath = nullptr);
    ~TextEditorWindow() override;

    void OnImGui() override;

    void SetSelection(int startLine, int startChar, int endLine, int endChar);
    void CenterViewAtLine(int line);
    const char* GetAssociatedFile();

    void OnReloadCommand();
    void OnLoadFromCommand();
    void OnSaveCommand();

    TextEditor *editor;
    bool has_associated_file = false;
    std::string associated_file;
    int tab_size = 4;
    float line_spacing = 1.0f;
    int undo_index_in_disk = 0;
    //int codeFontSize;

    char ctrlf_text_to_find[FIND_POPUP_TEXT_FIELD_LENGTH] = "";
    bool ctrlf_case_sensitive = false;
    bool requesting_load_from = false;
};

}
#endif // GZBTEXTEDITORWINDOW_HPP
