#ifndef PYWINDOW_HPP
#define PYWINDOW_HPP
#ifdef PYTHON3_FOUND

#include "app/Window.hpp"
#include "imgui.h"
#include "Python.h"

namespace gzb {
struct PyWindow : public Window
{
    char                  InputBuf[256];
    ImVector<char*>       Items;
    ImVector<const char*> Commands;
    ImVector<char*>       History;
    int                   HistoryPos;    // -1: new line, 0..History.Size-1 browsing history.
    ImGuiTextFilter       Filter;
    bool                  AutoScroll;
    bool                  ScrollToBottom;

    PyWindow()
    {
        window_name = "Python Window";
        showing=false;
        ClearLog();
        memset(InputBuf, 0, sizeof(InputBuf));
        HistoryPos = -1;

        PyGILState_STATE gstate;
        gstate = PyGILState_Ensure();
        // Import the keyword module and get the list of keywords
        PyObject* keyword_module = PyImport_ImportModule("keyword");
        if (keyword_module) {
            PyObject* keyword_list = PyObject_GetAttrString(keyword_module, "kwlist");
            if (keyword_list && PyList_Check(keyword_list)) {
                Py_ssize_t num_keywords = PyList_Size(keyword_list);
                for (Py_ssize_t i = 0; i < num_keywords; ++i) {
                    PyObject* keyword = PyList_GetItem(keyword_list, i);
                    if (PyUnicode_Check(keyword)) {
                        const char* keyword_cstr = PyUnicode_AsUTF8(keyword);
                        if (keyword_cstr) {
                            Commands.push_back(keyword_cstr);
                        }
                    }
                }
            }
            Py_XDECREF(keyword_list);
            Py_DECREF(keyword_module);
        }
        PyGILState_Release(gstate);

        AutoScroll = true;
        ScrollToBottom = false;
        ExecCommand("import sys");
        ExecCommand("sys.version");
    }

    ~PyWindow()
    {
        ClearLog();
        for (int i = 0; i < History.Size; i++)
            free(History[i]);
    }

    // Portable helpers
    static int   Stricmp(const char* s1, const char* s2)         { int d; while ((d = toupper(*s2) - toupper(*s1)) == 0 && *s1) { s1++; s2++; } return d; }
    static int   Strnicmp(const char* s1, const char* s2, int n) { int d = 0; while (n > 0 && (d = toupper(*s2) - toupper(*s1)) == 0 && *s1) { s1++; s2++; n--; } return d; }
    static char* Strdup(const char* s)                           { IM_ASSERT(s); size_t len = strlen(s) + 1; void* buf = malloc(len); IM_ASSERT(buf); return (char*)memcpy(buf, (const void*)s, len); }
    static void  Strtrim(char* s)                                { char* str_end = s + strlen(s); while (str_end > s && str_end[-1] == ' ') str_end--; *str_end = 0; }

    void    ClearLog()
    {
        for (int i = 0; i < Items.Size; i++)
            free(Items[i]);
        Items.clear();
    }

    void    AddLog(const char* fmt, ...) IM_FMTARGS(2)
    {
        // FIXME-OPT
        char buf[1024];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, IM_ARRAYSIZE(buf), fmt, args);
        buf[IM_ARRAYSIZE(buf)-1] = 0;
        va_end(args);
        Items.push_back(strdup(buf));
    }

    void OnImGui()
    {
        ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin(window_name.c_str(), &showing))
        {
            ImGui::End();
            return;
        }

        // As a specific feature guaranteed by the library, after calling Begin() the last Item represent the title bar.
        // So e.g. IsItemHovered() will return true when hovering the title bar.
        // Here we create a context menu only available from the title bar.
        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Close Console"))
                showing = false;
            ImGui::EndPopup();
        }

        //ImGui::TextWrapped(
        //    "This example implements a console with basic coloring, completion (TAB key) and history (Up/Down keys). A more elaborate "
        //    "implementation may want to store entries along with extra data such as timestamp, emitter, etc.");
        //ImGui::TextWrapped("Enter 'HELP' for help.");

        // TODO: display items starting from the bottom
        //ImGui::Text("system path");

        //if (ImGui::SmallButton("Add Debug Text"))
        //{
        //    AddLog("%d some text", Items.Size); AddLog("some more text"); AddLog("display very important message here!");
        //}
        //if (ImGui::SmallButton("Add Debug Error")) { AddLog("[error] something went wrong"); }
        //ImGui::SameLine();
        if (ImGui::SmallButton("Clear"))           { ClearLog(); }
        ImGui::SameLine();
        bool copy_to_clipboard = ImGui::SmallButton("Copy");
        //static float t = 0.0f; if (ImGui::GetTime() - t > 0.02f) { t = ImGui::GetTime(); AddLog("Spam %f", t); }

        ImGui::Separator();

        // Options menu
        if (ImGui::BeginPopup("Options"))
        {
            ImGui::Checkbox("Auto-scroll", &AutoScroll);
            ImGui::EndPopup();
        }

        // Options, Filter
        if (ImGui::Button("Options"))
            ImGui::OpenPopup("Options");
        ImGui::SameLine();
        Filter.Draw("Filter (\"incl,-excl\") (\"error\")", 180);
        ImGui::Separator();

        // Reserve enough left-over height for 1 separator + 1 input text
        const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
        if (ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar))
        {
            if (ImGui::BeginPopupContextWindow())
            {
                if (ImGui::Selectable("Clear")) ClearLog();
                ImGui::EndPopup();
            }

            // Display every line as a separate entry so we can change their color or add custom widgets.
            // If you only want raw text you can use ImGui::TextUnformatted(log.begin(), log.end());
            // NB- if you have thousands of entries this approach may be too inefficient and may require user-side clipping
            // to only process visible items. The clipper will automatically measure the height of your first item and then
            // "seek" to display only items in the visible area.
            // To use the clipper we can replace your standard loop:
            //      for (int i = 0; i < Items.Size; i++)
            //   With:
            //      ImGuiListClipper clipper;
            //      clipper.Begin(Items.Size);
            //      while (clipper.Step())
            //         for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
            // - That your items are evenly spaced (same height)
            // - That you have cheap random access to your elements (you can access them given their index,
            //   without processing all the ones before)
            // You cannot this code as-is if a filter is active because it breaks the 'cheap random-access' property.
            // We would need random-access on the post-filtered list.
            // A typical application wanting coarse clipping and filtering may want to pre-compute an array of indices
            // or offsets of items that passed the filtering test, recomputing this array when user changes the filter,
            // and appending newly elements as they are inserted. This is left as a task to the user until we can manage
            // to improve this example code!
            // If your items are of variable height:
            // - Split them into same height items would be simpler and facilitate random-seeking into your list.
            // - Consider using manual call to IsRectVisible() and skipping extraneous decoration from your items.
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // Tighten spacing
            if (copy_to_clipboard)
                ImGui::LogToClipboard();
            for (const char* item : Items)
            {
                if (!Filter.PassFilter(item))
                    continue;

                // Normally you would store more information in your item than just a string.
                // (e.g. make Items[] an array of structure, store color/type etc.)
                ImVec4 color;
                bool has_color = false;
                if (strstr(item, "[error]")) { color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f); has_color = true; }
                else if (strncmp(item, "# ", 2) == 0) { color = ImVec4(1.0f, 0.8f, 0.6f, 1.0f); has_color = true; }
                if (has_color)
                    ImGui::PushStyleColor(ImGuiCol_Text, color);
                ImGui::TextUnformatted(item);
                if (has_color)
                    ImGui::PopStyleColor();
            }
            if (copy_to_clipboard)
                ImGui::LogFinish();

            // Keep up at the bottom of the scroll region if we were already at the bottom at the beginning of the frame.
            // Using a scrollbar or mouse-wheel will take away from the bottom edge.
            if (ScrollToBottom || (AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
                ImGui::SetScrollHereY(1.0f);
            ScrollToBottom = false;

            ImGui::PopStyleVar();
        }
        ImGui::EndChild();
        ImGui::Separator();

        // Command-line
        bool reclaim_focus = false;
        ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_EscapeClearsAll | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory;
        if (ImGui::InputText("Input", InputBuf, IM_ARRAYSIZE(InputBuf), input_text_flags, &TextEditCallbackStub, (void*)this))
        {
            char* s = InputBuf;
            Strtrim(s);
            if (s[0])
                ExecCommand(s);
            strcpy(s, "");
            reclaim_focus = true;
        }

        // Auto-focus on window apparition
        ImGui::SetItemDefaultFocus();
        if (reclaim_focus)
            ImGui::SetKeyboardFocusHere(-1); // Auto focus previous widget

        ImGui::End();
    }

    // Function to capture the error as a string
    static void capture_error_as_string(char *buffer, size_t buffer_size) {
        PyObject *ptype, *pvalue, *ptraceback;

        // Fetch the error details
        PyErr_Fetch(&ptype, &pvalue, &ptraceback);
        PyErr_NormalizeException(&ptype, &pvalue, &ptraceback);

        if (pvalue != NULL) {
            PyObject *pyStr = PyObject_Str(pvalue);
            if (pyStr != NULL) {
                const char *error_message = PyUnicode_AsUTF8(pyStr);
                if (error_message != NULL) {
                    strncpy(buffer, error_message, buffer_size - 1);
                    buffer[buffer_size - 1] = '\0';  // Ensure null-termination
                }
                Py_DECREF(pyStr);
            }
        }

        // Clean up
        Py_XDECREF(ptype);
        Py_XDECREF(pvalue);
        Py_XDECREF(ptraceback);
    }

    void ExecCommand(const char* command_line)
    {
        // Check whether to call directly or surrounded by str()
        // Rule 1: import calls
        char *exe_cmd = NULL;
        int start = Py_eval_input;
        if (strstr(command_line, "import ") == command_line)
        {
            // No need to modify command_line
            exe_cmd = strdup(command_line);
            start = Py_single_input;
        }
        // rule 2 containing (
        else if (strlen(command_line) > 0 && strchr(command_line, '(') == NULL)
        {
            exe_cmd = (char*)malloc(strlen(command_line) + 6); // "str()" + NULL terminator
            sprintf(exe_cmd, "str(%s)", command_line);
        }
        // Rule 3: Function Calls with parentheses
        else if (strstr(command_line, "()") != NULL)
        {
            // No need to modify command_line
            exe_cmd = strdup(command_line);
        }
        // Rule 4: Other Cases
        else
        {
            // No need to modify command_line
            exe_cmd = strdup(command_line);
        }


        PyGILState_STATE gstate;
        gstate = PyGILState_Ensure();
        // Clear any previous errors
        PyErr_Clear();
        // Execute the user input as Python code
        //PyObject *g = PyEval_GetGlobals();
        //PyObject *l = PyEval_GetLocals();
        // Create global and local dictionaries for the Python execution context
        static PyObject *globals = PyDict_New();
        static PyObject *locals = PyDict_New();
        // Add the built-in modules to the global dictionary
        PyDict_SetItemString(globals, "__builtins__", PyEval_GetBuiltins());

        PyObject *pValue = PyRun_String(exe_cmd, start, globals, locals);
        free(exe_cmd);
        // Check for errors
        char error_message[1024] = {0};
        if (pValue == NULL)
        {
            //PyErr_Print();
            capture_error_as_string(error_message, sizeof(error_message));
            AddLog("%s", error_message);
        }
        else
        {
            PyObject *pyStr = PyObject_Str(pValue);
            if (pyStr != NULL)
            {
                const char *result_cstr = PyUnicode_AsUTF8(pyStr);
                if (result_cstr != NULL)
                    AddLog("%s", result_cstr);
                Py_DECREF(pyStr);
            }
            // If there is no error, decrease the reference count of pValue
            Py_XDECREF(pValue);
        }
        PyGILState_Release(gstate);

        AddLog("# %s\n", command_line);

        // Insert into history. First find match and delete it so it can be pushed to the back.
        // This isn't trying to be smart or optimal.
        HistoryPos = -1;
        for (int i = History.Size - 1; i >= 0; i--)
        {
            if (Stricmp(History[i], command_line) == 0)
            {
                free(History[i]);
                History.erase(History.begin() + i);
                break;
            }
        }
        History.push_back(Strdup(command_line));

        // On command input, we scroll to bottom even if AutoScroll==false
        ScrollToBottom = true;
    }

    // In C++11 you'd be better off using lambdas for this sort of forwarding callbacks
    static int TextEditCallbackStub(ImGuiInputTextCallbackData* data)
    {
        PyWindow* console = (PyWindow*)data->UserData;
        return console->TextEditCallback(data);
    }

    int TextEditCallback(ImGuiInputTextCallbackData* data)
    {
        //AddLog("cursor: %d, selection: %d-%d", data->CursorPos, data->SelectionStart, data->SelectionEnd);
        switch (data->EventFlag)
        {
        case ImGuiInputTextFlags_CallbackCompletion:
        {
            // Example of TEXT COMPLETION

            // Locate beginning of current word
            const char* word_end = data->Buf + data->CursorPos;
            const char* word_start = word_end;
            while (word_start > data->Buf)
            {
                const char c = word_start[-1];
                if (c == ' ' || c == '\t' || c == ',' || c == ';')
                    break;
                word_start--;
            }

            // Build a list of candidates
            ImVector<const char*> candidates;
            for (int i = 0; i < Commands.Size; i++)
                if (Strnicmp(Commands[i], word_start, (int)(word_end - word_start)) == 0)
                    candidates.push_back(Commands[i]);

            if (candidates.Size == 0)
            {
                // No match
                AddLog("No match for \"%.*s\"!\n", (int)(word_end - word_start), word_start);
            }
            else if (candidates.Size == 1)
            {
                // Single match. Delete the beginning of the word and replace it entirely so we've got nice casing.
                data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
                data->InsertChars(data->CursorPos, candidates[0]);
                data->InsertChars(data->CursorPos, " ");
            }
            else
            {
                // Multiple matches. Complete as much as we can..
                // So inputing "C"+Tab will complete to "CL" then display "CLEAR" and "CLASSIFY" as matches.
                int match_len = (int)(word_end - word_start);
                for (;;)
                {
                    int c = 0;
                    bool all_candidates_matches = true;
                    for (int i = 0; i < candidates.Size && all_candidates_matches; i++)
                        if (i == 0)
                            c = toupper(candidates[i][match_len]);
                        else if (c == 0 || c != toupper(candidates[i][match_len]))
                            all_candidates_matches = false;
                    if (!all_candidates_matches)
                        break;
                    match_len++;
                }

                if (match_len > 0)
                {
                    data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
                    data->InsertChars(data->CursorPos, candidates[0], candidates[0] + match_len);
                }

                // List matches
                AddLog("Possible matches:\n");
                for (int i = 0; i < candidates.Size; i++)
                    AddLog("- %s\n", candidates[i]);
            }

            break;
        }
        case ImGuiInputTextFlags_CallbackHistory:
        {
            // Example of HISTORY
            const int prev_history_pos = HistoryPos;
            if (data->EventKey == ImGuiKey_UpArrow)
            {
                if (HistoryPos == -1)
                    HistoryPos = History.Size - 1;
                else if (HistoryPos > 0)
                    HistoryPos--;
            }
            else if (data->EventKey == ImGuiKey_DownArrow)
            {
                if (HistoryPos != -1)
                    if (++HistoryPos >= History.Size)
                        HistoryPos = -1;
            }

            // A better implementation would preserve the data on the current input line along with cursor position.
            if (prev_history_pos != HistoryPos)
            {
                const char* history_str = (HistoryPos >= 0) ? History[HistoryPos] : "";
                data->DeleteChars(0, data->BufTextLen);
                data->InsertChars(0, history_str);
            }
        }
        } // switch end
        return 0;
    }

};
} // namespace
#endif // PYTHON3_FOUND
#endif // PYWINDOW_HPP
