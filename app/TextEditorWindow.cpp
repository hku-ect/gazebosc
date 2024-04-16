// mostly ad(o|a)pted from scex
#include "TextEditorWindow.hpp"
#include "../ext/ImFileDialog/ImFileDialog.h"
#include <filesystem>
#include <fstream>

namespace gzb
{
TextEditorWindow::TextEditorWindow(const char* filePath)
{
    editor = new TextEditor();
    if (filePath == nullptr)
    {
        // not sure if the pointer is a safe ID
        window_name = "unsaved###" + std::to_string((long)this);
        associated_file.clear();
    }
    else
    {
        has_associated_file = true;
        associated_file = std::string(filePath);
        auto pathObject = std::filesystem::path(filePath);
        window_name = pathObject.filename().string() + "###" + std::to_string((long)this);
        std::ifstream t(filePath);
        std::string str((std::istreambuf_iterator<char>(t)),
                        std::istreambuf_iterator<char>());
        editor->SetText(str);
    }
    editor->SetLanguageDefinition(TextEditor::LanguageDefinition::Python());
}

TextEditorWindow::~TextEditorWindow()
{
    delete editor;
}

void TextEditorWindow::OnImGui()
{
    ImGui::SetNextWindowSize(ImVec2(0.f, 200), ImGuiCond_FirstUseEver);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.0f, 0.0f });
    ImGui::Begin(window_name.c_str(), &showing,
                ImGuiWindowFlags_MenuBar |
                (editor->GetUndoIndex() != undo_index_in_disk ? ImGuiWindowFlags_UnsavedDocument : 0x0));
    ImGui::PopStyleVar();

    bool isFocused = ImGui::IsWindowFocused();
    bool requestingGoToLinePopup = false;
    bool requestingFindPopup = false;

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (has_associated_file && ImGui::MenuItem("Reload", "Ctrl+R"))
                OnReloadCommand();
            if (ImGui::MenuItem("Load from"))
            {
                requesting_load_from = true;
                OnLoadFromCommand();
            }
            if (ImGui::MenuItem("Save", "Ctrl+S"))
                OnSaveCommand();
            if (ImGui::MenuItem("Close", "Ctrl+W"))
                requesting_close = true;

            //if (this->has_associated_file && ImGui::MenuItem("Show in file explorer"))
            //Utils::ShowInFileExplorer(this->associated_file);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit"))
        {
            bool ro = editor->IsReadOnly();
            if (ImGui::MenuItem("Read only", nullptr, &ro))
                editor->SetReadOnly(ro);
            //bool ai = editor->IsAutoIndentEnabled();
            //if (ImGui::MenuItem("Auto indent on enter enabled", nullptr, &ai))
            //	editor->SetAutoIndentEnabled(ai);
            ImGui::Separator();

            if (ImGui::MenuItem("Undo", "ALT-Backspace", nullptr, !ro && editor->CanUndo()))
                editor->Undo();
            if (ImGui::MenuItem("Redo", "Ctrl+Y", nullptr, !ro && editor->CanRedo()))
                editor->Redo();

            ImGui::Separator();

            if (ImGui::MenuItem("Copy", "Ctrl+C", nullptr, editor->HasSelection()))
                editor->Copy();
            if (ImGui::MenuItem("Cut", "Ctrl+X", nullptr, !ro && editor->HasSelection()))
                editor->Cut();
            if (ImGui::MenuItem("Paste", "Ctrl+V", nullptr, !ro && ImGui::GetClipboardText() != nullptr))
                editor->Paste();

            ImGui::Separator();

            if (ImGui::MenuItem("Select all", nullptr, nullptr))
                editor->SelectAll();

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View"))
        {
            //ImGui::SliderInt("Font size", &codeFontSize, FontManager::GetMinCodeFontSize(), FontManager::GetMaxCodeFontSize());
            ImGui::SliderInt("Tab size", &tab_size, 1, 8);
            ImGui::SliderFloat("Line spacing", &line_spacing, 1.0f, 2.0f);
            editor->SetTabSize(tab_size);
            //editor->Setline_spacing(line_spacing);
            static bool showSpaces = editor->IsShowingWhitespaces();
            if (ImGui::MenuItem("Show spaces", nullptr, &showSpaces))
                editor->SetShowWhitespaces(!(editor->IsShowingWhitespaces()));
            //static bool showLineNumbers = editor->IsShowLineNumbersEnabled();
            //if (ImGui::MenuItem("Show line numbers", nullptr, &showLineNumbers))
            //	editor->SetShowLineNumbersEnabled(!(editor->IsShowLineNumbersEnabled()));
            static bool showShortTabs = editor->IsShowingShortTabGlyphs();
            if (ImGui::MenuItem("Short tabs", nullptr, &showShortTabs))
                editor->SetShowShortTabGlyphs(!(editor->IsShowingShortTabGlyphs()));
            /*if (ImGui::BeginMenu("Language"))
            {
                for (int i = (int)TextEditor::LanguageDefinitionId::None; i <= (int)TextEditor::LanguageDefinitionId::Hlsl; i++)
                {
                    bool isSelected = i == (int)editor->GetLanguageDefinition();
                    if (ImGui::MenuItem(languageDefinitionToName[(TextEditor::LanguageDefinitionId)i], nullptr, &isSelected))
                        editor->SetLanguageDefinition((TextEditor::LanguageDefinitionId)i);
                }
                ImGui::EndMenu();
            }*/
            /*if (ImGui::BeginMenu("Color scheme"))
            {
                for (int i = (int)TextEditor::PaletteId::Dark; i <= (int)TextEditor::PaletteId::RetroBlue; i++)
                {
                    bool isSelected = i == (int)editor->GetPalette();
                    if (ImGui::MenuItem(colorPaletteToName[(TextEditor::PaletteId)i], nullptr, &isSelected))
                        editor->SetPalette((TextEditor::PaletteId)i);
                }
                ImGui::EndMenu();
            }*/
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Find"))
        {
            if (ImGui::MenuItem("Go to line", "Ctrl+G"))
                requestingGoToLinePopup = true;
            if (ImGui::MenuItem("Find", "Ctrl+F"))
                requestingFindPopup = true;
            ImGui::EndMenu();
        }

        int line, column;
        TextEditor::Coordinates cursorCoord = editor->GetCursorPosition();
        line = cursorCoord.mLine;
        column = cursorCoord.mColumn;

        //if (codeFontTopBar != nullptr) ImGui::PushFont(codeFontTopBar);
        ImGui::Text("%6d/%-6d %6d lines | %s | %s", line + 1, column + 1, editor->GetTotalLines(),
                    editor->IsOverwrite() ? "Ovr" : "Ins",
                    editor->GetLanguageDefinitionName());
        //if (codeFontTopBar != nullptr) ImGui::PopFont();

        ImGui::EndMenuBar();
    }

    if (ifd::FileDialog::Instance().IsDone(window_name + "LoadFromDialog")) {
        if (ifd::FileDialog::Instance().HasResult()) {
            std::string res = ifd::FileDialog::Instance().GetResult().u8string();
            printf("TextEditor LoadFrom[%s]\n", res.c_str());
            associated_file = res;
            has_associated_file = true;
            OnLoadFromCommand();
        }
        ifd::FileDialog::Instance().Close();
    }

    if (ifd::FileDialog::Instance().IsDone(window_name + "SaveDialog")) {
        if (ifd::FileDialog::Instance().HasResult()) {
            std::string res = ifd::FileDialog::Instance().GetResult().u8string();
            printf("TextEditor Save[%s]\n", res.c_str());
            associated_file = res;
            has_associated_file = true;
            OnSaveCommand();
        }
        ifd::FileDialog::Instance().Close();
    }

    //if (codeFontEditor != nullptr) ImGui::PushFont(codeFontEditor);
    isFocused |= editor->Render("TextEditor", isFocused);
    //if (codeFontEditor != nullptr) ImGui::PopFont();

    if (isFocused)
    {
        bool ctrlPressed = ImGui::GetIO().KeyCtrl;
        if (ctrlPressed)
        {
            if (ImGui::IsKeyDown(ImGuiKey_S))
                OnSaveCommand();
            if (ImGui::IsKeyDown(ImGuiKey_R))
                OnReloadCommand();
            if (ImGui::IsKeyDown(ImGuiKey_G))
                requestingGoToLinePopup = true;
            if (ImGui::IsKeyDown(ImGuiKey_F))
                requestingFindPopup = true;
            //if (ImGui::IsKeyPressed(ImGuiKey_Equal) || ImGui::GetIO().MouseWheel > 0.0f)
            //	requestingFontSizeIncrease = true;
            //if (ImGui::IsKeyPressed(ImGuiKey_Minus) || ImGui::GetIO().MouseWheel < 0.0f)
            //	requestingFontSizeDecrease = true;
        }
    }

    if (requestingGoToLinePopup) ImGui::OpenPopup("go_to_line_popup");
    if (ImGui::BeginPopup("go_to_line_popup"))
    {
        static int targetLine;
        ImGui::SetKeyboardFocusHere();
        ImGui::InputInt("Line", &targetLine);
        if (ImGui::IsKeyDown(ImGuiKey_Enter) || ImGui::IsKeyDown(ImGuiKey_KeypadEnter))
        {
            static int targetLineFixed;
            targetLineFixed = targetLine < 1 ? 0 : targetLine - 1;
            editor->ClearExtraCursors();
            editor->ClearSelections();
            editor->SetCursorPosition( targetLineFixed, 0, -1);
            ImGui::CloseCurrentPopup();
            ImGui::GetIO().ClearInputKeys();
        }
        else if (ImGui::IsKeyDown(ImGuiKey_Escape))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    if (requestingFindPopup)
        ImGui::OpenPopup("find_popup");
    if (ImGui::BeginPopup("find_popup"))
    {
        ImGui::Checkbox("Case sensitive", &ctrlf_case_sensitive);
        if (requestingFindPopup)
            ImGui::SetKeyboardFocusHere();
        ImGui::InputText("To find", ctrlf_text_to_find, FIND_POPUP_TEXT_FIELD_LENGTH, ImGuiInputTextFlags_AutoSelectAll);
        int toFindTextSize = strlen(ctrlf_text_to_find);
        if ((ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter)) && toFindTextSize > 0)
        {
            editor->ClearExtraCursors();
            editor->SelectNextOccurrenceOf(ctrlf_text_to_find, toFindTextSize, ctrlf_case_sensitive);
            TextEditor::Coordinates nextOccurrenceLine = editor->GetCursorPosition();
        }
        //if (ImGui::Button("Find all") && toFindTextSize > 0)
        //	editor->SelectAllOccurrencesOf(ctrlf_text_to_find, toFindTextSize, ctrlf_case_sensitive);
        else if (ImGui::IsKeyDown(ImGuiKey_Escape))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }
    if (requesting_close)
    {
        if (editor->GetUndoIndex() )
            ImGui::OpenPopup("requesting_close_popup");
        else
            requesting_destroy = true;
        if (ImGui::BeginPopup("requesting_close_popup"))
        {
            ImGui::Text("Usaved changes will be lost. Are you sure?");
            if ( ImGui::Button("Yes") )
            {
                requesting_destroy = true;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if ( ImGui::Button("No"))
            {
                requesting_close = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
}

    //if (requestingFontSizeIncrease && codeFontSize < FontManager::GetMaxCodeFontSize())
    //	codeFontSize++;
    //if (requestingFontSizeDecrease && codeFontSize > FontManager::GetMinCodeFontSize())
    //	codeFontSize--;

    ImGui::End();
}

const char* TextEditorWindow::GetAssociatedFile()
{
    if (!has_associated_file)
        return nullptr;
    return associated_file.c_str();
}

// Commands

void TextEditorWindow::OnReloadCommand()
{
    std::ifstream t(associated_file);
    std::string str((std::istreambuf_iterator<char>(t)),
                    std::istreambuf_iterator<char>());
    editor->SetText(str);
    undo_index_in_disk = 0;
}

void TextEditorWindow::OnLoadFromCommand()
{
    if (requesting_load_from)
    {
        ifd::FileDialog::Instance().Open(window_name + "LoadFromDialog", "Load text from file", "*.py;*txt {.py,.txt}");
        requesting_load_from = false;
        return;
    }
    std::ifstream t(associated_file);
    std::string str((std::istreambuf_iterator<char>(t)),
                    std::istreambuf_iterator<char>());
    editor->SetText(str);
    window_name = std::filesystem::path(associated_file).filename().string() + "###" + std::to_string((long)this);
    //auto pathObject = std::filesystem::path(associated_file);
    //auto lang = extensionToLanguageDefinition.find(pathObject.extension().string());
    //if (lang != extensionToLanguageDefinition.end())
    //	editor->SetLanguageDefinition(extensionToLanguageDefinition[pathObject.extension().string()]);
    undo_index_in_disk = -1; // assume they are loading text from some other file
}

void TextEditorWindow::OnSaveCommand()
{
    if (!has_associated_file || associated_file == "")
    {
        ifd::FileDialog::Instance().Save(window_name+"SaveDialog", "Save text file", "*.py;*.txt {.py,.txt}");
        return;
    }
    std::string textToSave = editor->GetText();
    window_name = std::filesystem::path(associated_file).filename().string() + "##" + std::to_string((long)this);
    std::ofstream outFile(associated_file, std::ios::binary);
    outFile << textToSave;
    outFile.close();
    undo_index_in_disk = editor->GetUndoIndex();
}

} // namespace
