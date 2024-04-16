#include "ActorContainer.hpp"
#include "App.hpp"
#include "ext/ImFileDialog/ImFileDialog.h"

namespace gzb {

ActorContainer::ActorContainer(sphactor_t *actor) {
    this->actor = actor;
    this->title = sphactor_ask_actor_type(actor);
    this->capabilities = zconfig_dup(sphactor_capability(actor));
    this->pos.x = sphactor_position_x(actor);
    this->pos.y = sphactor_position_y(actor);

    // retrieve in-/output sockets
    if (this->capabilities)
    {
        zconfig_t *insocket = zconfig_locate( this->capabilities, "inputs/input");
        if ( insocket !=  nullptr )
        {
            zconfig_t *type = zconfig_locate(insocket, "type");
            assert(type);

            char* typeStr = zconfig_value(type);
            if ( streq(typeStr, "OSC")) {
                input_slots.push_back({ "OSC", ActorSlotOSC });
            }
            else if ( streq(typeStr, "NatNet")) {
                input_slots.push_back({ "NatNet", ActorSlotNatNet });
            }
            else if ( streq(typeStr, "Any")) {
                input_slots.push_back({ "Any", ActorSlotAny });
            }
            else {
                zsys_error("Unsupported input type: %s", typeStr);
            }
        }

        zconfig_t *outsocket = zconfig_locate( this->capabilities, "outputs/output");

        if ( outsocket !=  nullptr )
        {
            zconfig_t *type = zconfig_locate(outsocket, "type");
            assert(type);

            char* typeStr = zconfig_value(type);
            if ( streq(typeStr, "OSC")) {
                output_slots.push_back({ "OSC", ActorSlotOSC });
            }
            else if ( streq(typeStr, "NatNet")) {
                output_slots.push_back({ "NatNet", ActorSlotNatNet });
            }
            else if ( streq(typeStr, "Any")) {
                output_slots.push_back({ "Any", ActorSlotAny });
            }
            else {
                zsys_error("Unsupported output type: %s", typeStr);
            }
        }
    }
    else
    {
        input_slots.push_back({ "OSC", ActorSlotOSC });
        output_slots.push_back({ "OSC", ActorSlotOSC });
    }
    //ParseConnections();
    //InitializeCapabilities(); //already done by sph_stage?
}

ActorContainer::~ActorContainer() {
    zconfig_destroy(&this->capabilities);
}

void ActorContainer::ParseConnections() {
    if ( this->capabilities == NULL ) return;

    // get actor's connections
    zlist_t *conn = sphactor_connections( this->actor );
    for ( char *connection = (char *)zlist_first(conn); connection != nullptr; connection = (char *)zlist_next(conn))
    {
        Connection new_connection;
        new_connection.input_node = this;
        new_connection.output_node = FindActorContainerByEndpoint(connection);
        assert(new_connection.input_node);
        assert(new_connection.output_node);
        ((ActorContainer*) new_connection.input_node)->connections.push_back(new_connection);
        ((ActorContainer*) new_connection.output_node)->connections.push_back(new_connection);
    }
}

ActorContainer *
ActorContainer::FindActorContainerByEndpoint(const char *endpoint)
{
    return NULL;
}

void
ActorContainer::InitializeCapabilities() {
    if ( this->capabilities == NULL ) return;

    zconfig_t *root = zconfig_locate(this->capabilities, "capabilities");
    if ( root == NULL ) return;

    zconfig_t *data = zconfig_locate(root, "data");
    while( data != NULL ) {
        HandleAPICalls(data);
        data = zconfig_next(data);
    }
}

void
ActorContainer::SetCapabilities(const char* capabilities ) {
    this->capabilities = zconfig_str_load(capabilities);
    InitializeCapabilities();
}

#define GZB_TOOLTIP_THRESHOLD 1.0f
void
ActorContainer::RenderTooltip(const char *help)
{
    if ( strlen(help) )
    {
        if (ImGui::IsItemHovered() && GImGui->HoveredIdTimer > GZB_TOOLTIP_THRESHOLD )
        {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 24.0f);
            ImGui::TextUnformatted(help);
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }
}

void
ActorContainer::HandleAPICalls(zconfig_t * data) {
    zconfig_t *zapic = zconfig_locate(data, "api_call");
    if ( zapic ) {
        zconfig_t *zapiv = zconfig_locate(data, "api_value");
        zconfig_t *zvalue = zconfig_locate(data, "value");

        if (zapiv)
        {
            char * zapivStr = zconfig_value(zapiv);
            char type = zapivStr[0];
            switch( type ) {
            case 'b': {
                char *buf = new char[4];
                const char *zvalueStr = zconfig_value(zvalue);
                strcpy(buf, zvalueStr);
                sphactor_ask_api(this->actor, zconfig_value(zapic), zapivStr, zvalueStr);
                //SendAPI<char *>(zapic, zapiv, zvalue, &buf);
                zstr_free(&buf);
            } break;
            case 'i': {
                int ival;
                ReadInt(&ival, zvalue);
                sphactor_ask_api(this->actor, zconfig_value(zapic), zapivStr, gzb_itoa(ival));
            } break;
            case 'f': {
                float fval;
                ReadFloat(&fval, zvalue);
                //SendAPI<float>(zapic, zapiv, zvalue, &fval);
                sphactor_ask_api(this->actor, zconfig_value(zapic), zapivStr, ftoa(fval));
            } break;
            case 's': {
                const char *zvalueStr = zconfig_value(zvalue);
                //SendAPI<char *>(zapic, zapiv, zvalue, const_cast<char **>(&zvalueStr));
                sphactor_ask_api(this->actor, zconfig_value(zapic), zapivStr, zvalueStr);
            } break;
            }
        }
        else if (zvalue) {
            //assume string
            //int ival;
            //ReadInt(&ival, zvalue);
            //zsock_send( sphactor_socket(this->actor), "si", zconfig_value(zapic), ival);
            sphactor_ask_api(this->actor, zconfig_value(zapic), zconfig_value(zapic), zconfig_value(zvalue));
        }
    }
}

template<typename T>
void
ActorContainer::SendAPI(zconfig_t *zapic, zconfig_t *zapiv, zconfig_t *zvalue, T * value) {
    if ( !zapic ) return;

    if (zapiv)
    {
        std::string pic = "s";
        pic += zconfig_value(zapiv);
        //zsys_info("Sending: %s", pic.c_str());
        //sphactor_ask_api( this->actor, pic.c_str(), zconfig_value(zapic), *value);
        zsock_send( sphactor_socket(this->actor), pic.c_str(), zconfig_value(zapic), *value);
    }
    else
        //sphactor_ask_api( this->actor, "ss", zconfig_value(zapic), *value);
        zsock_send( sphactor_socket(this->actor), "si", zconfig_value(zapic), *value);
}


void
ActorContainer::Render() {
    //loop through each data element in capabilities
    if ( this->capabilities == NULL ) return;

    zconfig_t *root = zconfig_locate(this->capabilities, "capabilities");
    if ( root == NULL ) return;

    zconfig_t *data = zconfig_locate(root, "data");
    while( data != NULL ) {
        zconfig_t *name = zconfig_locate(data, "name");
        zconfig_t *type = zconfig_locate(data, "type");
        assert(name);
        assert(type);

        char* nameStr = zconfig_value(name);
        char* typeStr = zconfig_value(type);
        if ( streq(typeStr, "int")) {
            RenderInt( nameStr, data );
        }
        else if ( streq(typeStr, "slider")) {
            RenderSlider( nameStr, data );
        }
        else if ( streq(typeStr, "float")) {
            RenderFloat( nameStr, data );
        }
        else if ( streq(typeStr, "string")) {
            RenderString(nameStr, data);
        }
        else if ( streq(typeStr, "bool")) {
            RenderBool( nameStr, data );
        }
        else if ( streq(typeStr, "filename")) {
            RenderFilename( nameStr, data );
        }
        else if ( streq(typeStr, "mediacontrol")) {
            RenderMediacontrol( nameStr, data );
        }
        else if ( streq(typeStr, "list")) {
            RenderMultilineString( nameStr, data );
        }
        else if ( streq(typeStr, "trigger")) {
            RenderTrigger( nameStr, data );
        }

        data = zconfig_next(data);
    }
    RenderCustomReport();
}

void
ActorContainer::RenderCustomReport() {
    const int LABEL_WIDTH = 25;
    const int VALUE_WIDTH = 50;

    sphactor_report_t * report = sphactor_report(actor);
    lastActive = sphactor_report_send_time(report);

    assert(report);
    zosc_t * customData = sphactor_report_custom(report);
    if ( customData ) {
        const char* address = zosc_address(customData);
        //ImGui::Text("%s", address);

        char type = '0';
        const void *data = zosc_first(customData, &type);
        bool name = true;
        char* nameStr = nullptr;
        while( data ) {
            if ( name ) {
                //expecting a name string for each piece of data
                assert(type == 's');

                int rc = zosc_pop_string(customData, &nameStr);
                if (rc == 0 )
                {
                    ImGui::BeginGroup();
                    if (!streq(nameStr, "lastActive")) {
                        ImGui::SetNextItemWidth(LABEL_WIDTH);
                        ImGui::Text("%s:", nameStr);
                        ImGui::SameLine();
                        ImGui::SetNextItemWidth(VALUE_WIDTH);
                    }
                }
            }
            else {
                switch(type) {
                case 's': {
                    char* value;
                    int rc = zosc_pop_string(customData, &value);
                    if( rc == 0)
                    {
                        ImGui::Text("%s", value);
                        zstr_free(&value);
                    }
                } break;
                case 'c': {
                    char value;
                    zosc_pop_char(customData, &value);
                    ImGui::Text("%c", value);
                } break;
                case 'i': {
                    int32_t value;
                    zosc_pop_int32(customData, &value);
                    ImGui::Text("%i", value);
                } break;
                case 'h': {
                    int64_t value;
                    zosc_pop_int64(customData, &value);

                    if (streq(nameStr, "lastActive")) {
                        // render something to indicate the actor is currently active
                        lastActive = (int64_t)value;
                        int64_t diff = zclock_mono() - lastActive;
                        ImDrawList* draw_list = ImGui::GetWindowDrawList();
                        ImNodes::CanvasState* canvas = ImNodes::GetCurrentCanvas();
                        if (diff < 100) {
                            draw_list->AddCircleFilled(canvas->Offset + pos * canvas->Zoom + ImVec2(5,5) * canvas->Zoom, 5 * canvas->Zoom , ImColor(0, 255, 0), 4);
                        }
                        else {
                            draw_list->AddCircleFilled(canvas->Offset + pos * canvas->Zoom + ImVec2(5, 5) * canvas->Zoom, 5 * canvas->Zoom, ImColor(127, 127, 127), 4);
                        }
                    }
                    else {
                        ImGui::Text("%li", value);
                    }
                } break;
                case 'f': {
                    float value;
                    zosc_pop_float(customData, &value);
                    ImGui::Text("%f", value);
                } break;
                case 'd': {
                    double value;
                    zosc_pop_double(customData, &value);
                    ImGui::Text("%f", value);
                } break;
                case 'F': {
                    ImGui::Text("FALSE");
                } break;
                case 'T': {
                    ImGui::Text("TRUE");
                } break;
                }

                ImGui::EndGroup();

                //free the nameStr here so we can use it up to this point
                zstr_free(&nameStr);
            }

            //flip expecting name or value
            name = !name;
            data = zosc_next(customData, &type);
        }
    }
}

void
ActorContainer::SolvePadding( int* position ) {
    if ( *position % 4 != 0 ) {
        *position += 4 - *position % 4;
    }
}

template<typename T>
void
ActorContainer::RenderValue(T *value, const byte * bytes, int *position) {
    memcpy(value, bytes+*position, sizeof(T));
    *position += sizeof(T);
}

// Make the UI compact because there are so many fields
void
ActorContainer::PushStyleCompact()
{
    ImGuiStyle& style = ImGui::GetStyle();
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(style.FramePadding.x, (float)(int)(style.FramePadding.y * 0.60f)));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(style.ItemSpacing.x, (float)(int)(style.ItemSpacing.y * 0.60f)));
}

void
ActorContainer::PopStyleCompact()
{
    ImGui::PopStyleVar(2);
}

void
ActorContainer::RenderList(const char *name, zconfig_t *data)
{
    ImVec2 size = ImVec2(300,100); // what's a reasonable size?
    ImGui::BeginChild("##", size, false, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoSavedSettings );
    // The list capability expects a tree of zconfig data.
    // Every node is a column
    zconfig_t *namec = zconfig_locate(data, "name");
    int colcount = 0;
    zconfig_t *columnc = zconfig_child(data);
    while (columnc)
    {
        // we only count nodes not leaves
        if ( zconfig_child(columnc) )
        {
            colcount++;
        }
        columnc = zconfig_next(columnc);
    }
    assert(colcount);

    // By default, if we don't enable ScrollX the sizing policy for each columns is "Stretch"
    // Each columns maintain a sizing weight, and they will occupy all available width.
    static ImGuiTableFlags flags = ImGuiTableFlags_Resizable ;//ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_ContextMenuInBody | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY;
    if (ImGui::BeginTable(zconfig_value(namec), colcount, flags))
    {
        unsigned int dirty = 0; //track changes bitwise
        columnc = zconfig_child(data);
        // generate table header
        while (columnc)
        {
            if ( zconfig_child(columnc) )
            {
                char *name = zconfig_name(columnc);
                ImGui::TableSetupColumn(name, ImGuiTableColumnFlags_None);
            }
            columnc = zconfig_next(columnc);
        }
        ImGui::TableHeadersRow();
        // generate entries

        // always end with an empty entry row
        char *values[10] = {NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL}; // capture input
        columnc = zconfig_child(data);
        ImGui::TableNextRow();
        int colidx = 0;
        while (columnc)
        {
            zconfig_t *rowc = zconfig_child(columnc);
            if ( rowc )
            {
                char *name = zconfig_name(columnc);
                ImGui::TableSetColumnIndex(colidx);
                zconfig_t *typec = zconfig_locate(columnc, "type");
                char *type = zconfig_value(typec);
                ImGui::PushItemWidth( -1.0f );
                if ( streq(type, "intbla") )
                {
                    int value = 6200;
                    if ( ImGui::InputInt("##", &value, 1, 100,ImGuiInputTextFlags_EnterReturnsTrue ) )
                    {
                        dirty = (1 << colidx) | colidx;
                    }
                }
                else
                {
                    static char bla[256] = "";
                    char label[256] = "";
                    snprintf(label, 256, "##%s", name);
                    if ( ImGui::InputText(&label[0], &bla[0], 256, ImGuiInputTextFlags_None) )
                    {
                        dirty = (1 << colidx) | colidx;
                    }
                }
                ImGui::PopItemWidth();
                colidx++;
            }
            columnc = zconfig_next(columnc);
        }
        ImGui::EndTable();

        if (dirty)
        {
            // get current value, append new value if all input fields contain data
        }
    }
    //PushStyleCompact();
    //ImGui::CheckboxFlags("ImGuiTableFlags_Resizable", &flags, ImGuiTableFlags_Resizable);
    //ImGui::CheckboxFlags("ImGuiTableFlags_BordersV", &flags, ImGuiTableFlags_BordersV);
    //ImGui::SameLine(); HelpMarker("Using the _Resizable flag automatically enables the _BordersInnerV flag as well, this is why the resize borders are still showing when unchecking this.");
    //PopStyleCompact();


    /*
        if (ImGui::BeginTable("table1", 3, flags))
        {
            ImGui::TableSetupColumn("name", ImGuiTableColumnFlags_None, 80.f);
            ImGui::TableSetupColumn("ip", ImGuiTableColumnFlags_None, 80.f);
            ImGui::TableSetupColumn("port", ImGuiTableColumnFlags_None, 60.f);
            ImGui::TableHeadersRow();
            for (int row = 0; row < 3; row++)
            {
                ImGui::TableNextRow();

                for (int column = 0; column < 3; column++)
                {
                    ImGui::TableSetColumnIndex(column);
                    static int port = 0;
                    static char bla[256] = "";
                    if (column == 2)
                        ImGui::InputInt("##port", &port);
                    else
                        ImGui::InputText("##Hello", &bla[0], 256);
                }
            }
            ImGui::EndTable();
        }*/
    ImGui::EndChild();
}

void
ActorContainer::RenderMediacontrol(const char* name, zconfig_t *data)
{
    if ( ImGui::Button(ICON_FA_BACKWARD) )
        //sphactor_ask_api(this->actor, "BACK", "", NULL);
        zstr_send(sphactor_socket(this->actor), "BACK");
    ImGui::SameLine();
    if ( ImGui::Button(ICON_FA_PLAY) )
        zstr_send(sphactor_socket(this->actor), "PLAY");
    ImGui::SameLine();
    if ( ImGui::Button(ICON_FA_PAUSE) )
        zstr_send(sphactor_socket(this->actor), "PAUSE");
}

void
ActorContainer::RenderFilename(const char* name, zconfig_t *data) {
    int max = MAX_STR_DEFAULT;

    zconfig_t * zvalue = zconfig_locate(data, "value");
    zconfig_t * ztype_hint = zconfig_locate(data, "type_hint");
    zconfig_t * zapic = zconfig_locate(data, "api_call");
    zconfig_t * zapiv = zconfig_locate(data, "api_value");
    zconfig_t * zoptions = zconfig_locate(data, "options");
    zconfig_t * zvalidfiles = zconfig_locate(data, "valid_files");
    assert(zvalue);

    zconfig_t * zmax = zconfig_locate(data, "max");

    ReadInt( &max, zmax );

    char buf[MAX_STR_DEFAULT];
    const char* zvalueStr = zconfig_value(zvalue);
    strcpy(buf, zvalueStr);
    char *p = &buf[0];
    bool file_selected = false;

    const char* zoptStr = "r";
    if ( zoptions != nullptr ) {
        zoptStr = zconfig_value(zoptions);
    }
    const char *valid_files = zvalidfiles == nullptr ? "*.*" : zconfig_value(zvalidfiles);

    ImGui::SetNextItemWidth(180);
    if ( ImGui::InputTextWithHint("", name, buf, max, ImGuiInputTextFlags_EnterReturnsTrue ) ) {
        zconfig_set_value(zvalue, "%s", buf);
        sphactor_ask_api(this->actor, zconfig_value(zapic), zconfig_value(zapiv), p );
        //SendAPI<char*>(zapic, zapiv, zvalue, &(p));
    }
    ImGui::SameLine();
    if ( ImGui::Button( ICON_FA_FOLDER_OPEN ) )
        file_selected = true;

    if ( file_selected )
    {
        char filter[1024];
        sprintf(filter, "Valid files: (%s){%s},.*", valid_files, valid_files);
        ifd::FileDialog::Instance().Open(std::string(title) + "ActorOpenDialog", "Actor Open File", filter);
    }

    if (ifd::FileDialog::Instance().IsDone(std::string(title) + "ActorOpenDialog"))
    {
        if (ifd::FileDialog::Instance().HasResult())
        {
            std::string res = ifd::FileDialog::Instance().GetResult().u8string();
            char *path = convert_to_relative_to_wd(res.c_str());
            zconfig_set_value(zvalue, "%s", path );
            strcpy(buf, path);
            //SendAPI<char*>(zapic, zapiv, zvalue, &(p) );
            zstr_free(&path);
            sphactor_ask_api(this->actor, zconfig_value(zapic), zconfig_value(zapiv), p );
        }
    }

    zconfig_t *help = zconfig_locate(data, "help");
    const char *helpv = "Load a file";
    if (help)
    {
        helpv = zconfig_value(help);
    }
    RenderTooltip(helpv);

    // handle options
    zconfig_t *opts = zconfig_locate(data, "options");
    // check if there are options
    if (opts)
    {
        char *optsv = zconfig_value(opts);
        // check if is an editable textfile
        if (optsv && strchr(optsv, 't') != NULL && strchr(optsv, 'e') != NULL)
        {
            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_EDIT))
            {
                zconfig_t* zvalue = zconfig_locate(data, "value");
                const char* zvalueStr = zconfig_value(zvalue);
                if (strlen(zvalueStr) && zsys_file_exists (zvalueStr) )
                {
                    OpenTextEditor(zvalueStr); // could own the f pointer
                }
                else
                    zsys_error("no valid file to load: %s", zvalueStr);
            }
            RenderTooltip("Edit file in texteditor");
        }
        if (optsv && strchr(optsv, 'c') != NULL && strchr(optsv, 't') != NULL) // we can create a new text file
        {
            // create new text files
            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_FILE))
                ImGui::OpenPopup("Create File?");

            RenderTooltip("Create new file");
            // Always center this window when appearing
            ImVec2 center = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

            if (ImGui::BeginPopupModal("Create File?", NULL, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::Text("Enter a filename:");
                ImGui::Separator();
                static char fnamebuf[PATH_MAX] = "";
                bool createFile = false;
                if ( ImGui::InputText("filename", fnamebuf, PATH_MAX, ImGuiInputTextFlags_EnterReturnsTrue ) )
                {
                    if ( strlen(fnamebuf) )
                    {
                        createFile = true;
                    }
                    ImGui::CloseCurrentPopup();
                }
                char feedback[256] = "Enter a valid filename!";
                if (strlen(fnamebuf))
                {
                    // check if this filename conflicts
                    char path[PATH_MAX];
                    getcwd(path, PATH_MAX);
                    char fullpath[PATH_MAX];
                    sprintf (fullpath, "%s/%s", path, fnamebuf);
                    //zsys_info("filemode %i", zsys_file_mode(fullpath));
                    int mode = zsys_file_mode(fullpath);
                    if ( mode != -1 )
                    {
                        if ( S_ISDIR(mode) )
                            sprintf( feedback, "Filename \'%s\' conflicts with an existing directory!", fnamebuf);
                        else
                            sprintf( feedback, "File %s exists and will be overwritten if continued", fnamebuf);
                    }
                }
                ImGui::PushTextWrapPos(ImGui::GetFontSize() * 24.0f);
                ImGui::Text("%s", feedback);
                ImGui::PopTextWrapPos();

                if (ImGui::Button("OK", ImVec2(120, 0)))
                {
                    if ( strlen(fnamebuf) )
                    {
                        createFile = true;
                    }
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SetItemDefaultFocus();
                ImGui::SameLine();
                if (ImGui::Button("Cancel", ImVec2(120, 0)))
                {
                    createFile = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();

                if (createFile && strlen(fnamebuf))
                {
                    zchunk_t *file_data = NULL;
                    zconfig_t *tmpl = zconfig_locate(data, "file_template");
                    if (tmpl)
                    {
                        char *tmplpath = zconfig_value(tmpl);
                        char fullpath[PATH_MAX];
                        snprintf(fullpath, PATH_MAX-1, "%s/%s", GZB_GLOBAL.RESOURCESPATH, tmplpath);
                        if ( zsys_file_exists(fullpath) )
                        {
                            zfile_t *tmplf = zfile_new(NULL, fullpath);
                            assert(tmplf);
                            int rc = zfile_input(tmplf);
                            assert(rc == 0);
                            file_data = zfile_read(tmplf, zsys_file_size (tmplpath), 0);
                            assert(file_data);
                            zfile_destroy(&tmplf);
                        }
                        else
                        {
                            file_data = zchunk_new ("\n", 1);
                        }
                    }
                    else
                    {
                        file_data = zchunk_new ("\n", 1);
                    }
                    // create the new textfile
                    char path[PATH_MAX];
                    getcwd(path, PATH_MAX);
                    zfile_t *txtfile = zfile_new(path, fnamebuf);
                    assert(txtfile);
                    int rc = zfile_output(txtfile);
                    assert(rc == 0);
                    rc = zfile_write (txtfile, file_data, 0);
                    assert (rc == 0);
                    zchunk_destroy (&file_data);
                    zfile_close(txtfile);
                    //zfile_destroy(&txtfile);
                    zconfig_set_value(zvalue, "%s", fnamebuf);
                    sphactor_ask_api(this->actor, zconfig_value(zapic), zconfig_value(zapiv), p );

                    OpenTextEditor(zfile_filename(txtfile, NULL)); // could own the f pointer
                }
            }
        }
    }
}

void
ActorContainer::RenderBool(const char* name, zconfig_t *data) {
    bool value;

    zconfig_t * zvalue = zconfig_locate(data, "value");
    zconfig_t * zapic = zconfig_locate(data, "api_call");
    zconfig_t * zapiv = zconfig_locate(data, "api_value");
    assert(zvalue);
    const char *apiv;
    // set default call value
    if (zapiv == nullptr)
        apiv = "s";
    else
        apiv = zconfig_value(zapiv);

    ReadBool( &value, zvalue);

    ImGui::SetNextItemWidth(100);
    if ( ImGui::Checkbox( name, &value ) ) {
        zconfig_set_value(zvalue, "%s", value ? "True" : "False");

        char *buf = new char[6];
        const char *zvalueStr = zconfig_value(zvalue);
        strcpy(buf, zvalueStr);
        //SendAPI<char *>(zapic, zapiv, zvalue, &buf);
        sphactor_ask_api(this->actor, zconfig_value(zapic), apiv, buf );
        zstr_free(&buf);
    }
    zconfig_t *help = zconfig_locate(data, "help");
    if (help)
    {
        char *helpv = zconfig_value(help);
        RenderTooltip(helpv);
    }
}

void
ActorContainer::RenderInt(const char* name, zconfig_t *data) {
    int value;
    int min = 0, max = 0, step = 0;

    zconfig_t * zvalue = zconfig_locate(data, "value");
    zconfig_t * zapic = zconfig_locate(data, "api_call");
    zconfig_t * zapiv = zconfig_locate(data, "api_value");
    assert(zvalue);

    zconfig_t * zmin = zconfig_locate(data, "min");
    zconfig_t * zmax = zconfig_locate(data, "max");
    zconfig_t * zstep = zconfig_locate(data, "step");

    ReadInt( &value, zvalue);
    ReadInt( &min, zmin);
    ReadInt( &max, zmax);
    ReadInt( &step, zstep);

    ImGui::SetNextItemWidth(100);

    if ( ImGui::InputInt( name, &value, step, 100) ) {
        if ( zmin ) {
            if (value < min) value = min;
        }
        if ( zmax ) {
            if ( value > max ) value = max;
        }

        zconfig_set_value(zvalue, "%i", value);
        //SendAPI<int>(zapic, zapiv, zvalue, &value);
        sphactor_ask_api(this->actor, zconfig_value(zapic), zconfig_value(zapiv), gzb_itoa(value) );
    }
    zconfig_t *help = zconfig_locate(data, "help");
    if (help)
    {
        char *helpv = zconfig_value(help);
        RenderTooltip(helpv);
    }
}

void
ActorContainer::RenderSlider(const char* name, zconfig_t *data) {

    zconfig_t * zvalue = zconfig_locate(data, "value");
    zconfig_t * zapic = zconfig_locate(data, "api_call");
    zconfig_t * zapiv = zconfig_locate(data, "api_value");
    assert(zvalue);

    zconfig_t * zmin = zconfig_locate(data, "min");
    zconfig_t * zmax = zconfig_locate(data, "max");
    zconfig_t * zstep = zconfig_locate(data, "step");

    ImGui::SetNextItemWidth(250);
    ImGui::BeginGroup();
    ImGui::SetNextItemWidth(200);

    if ( streq(zconfig_value(zapiv), "f") )
    {
        float value;
        float min = 0., max = 0., step = 0.;
        ReadFloat( &value, zvalue);
        ReadFloat( &min, zmin);
        ReadFloat( &max, zmax);
        ReadFloat( &step, zstep);

        if ( ImGui::SliderFloat("", &value, min, max) ) {
            zconfig_set_value(zvalue, "%f", value);
            sphactor_ask_api(this->actor, zconfig_value(zapic), zconfig_value(zapiv), ftoa(value) );
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(35);
        if ( ImGui::InputFloat("##min", &min, step, 0.f, "%.2f", ImGuiInputTextFlags_EnterReturnsTrue ) )
        {
            zconfig_set_value(zmin, "%f", min);
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(35);
        if ( ImGui::InputFloat( "##max", &max, step, 0.f, "%.2f", ImGuiInputTextFlags_EnterReturnsTrue ) )
        {
            zconfig_set_value(zmax, "%f", max);
        }
    }
    else
    {
        int value;
        int min = 0, max = 0, step = 0;
        ReadInt( &value, zvalue);
        ReadInt( &min, zmin);
        ReadInt( &max, zmax);
        ReadInt( &step, zstep);

        if ( ImGui::SliderInt( "", &value, min, max) ) {
            zconfig_set_value(zvalue, "%i", value);
            sphactor_ask_api(this->actor, zconfig_value(zapic), zconfig_value(zapiv), gzb_itoa(value) );
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(30);
        if ( ImGui::InputInt( "##min", &min, 0, 0,ImGuiInputTextFlags_EnterReturnsTrue ) )
        {
            zconfig_set_value(zmin, "%i", min);
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(30);
        if ( ImGui::InputInt( "##max", &max, 0, 0,ImGuiInputTextFlags_EnterReturnsTrue ) )
        {
            zconfig_set_value(zmax, "%i", max);
        }
    }

    ImGui::EndGroup();
    zconfig_t *help = zconfig_locate(data, "help");
    if (help)
    {
        char *helpv = zconfig_value(help);
        RenderTooltip(helpv);
    }
}

void
ActorContainer::RenderFloat(const char* name, zconfig_t *data) {
    float value;
    float min = 0, max = 0, step = 0;

    zconfig_t * zvalue = zconfig_locate(data, "value");
    zconfig_t * zapic = zconfig_locate(data, "api_call");
    zconfig_t * zapiv = zconfig_locate(data, "api_value");
    assert(zvalue);

    zconfig_t * zmin = zconfig_locate(data, "min");
    zconfig_t * zmax = zconfig_locate(data, "max");
    zconfig_t * zstep = zconfig_locate(data, "step");

    ReadFloat( &value, zvalue);
    ReadFloat( &min, zmin);
    ReadFloat( &max, zmax);
    ReadFloat( &step, zstep);

    ImGui::SetNextItemWidth(100);
    if ( min != max ) {
        if ( ImGui::SliderFloat( name, &value, min, max) ) {
            zconfig_set_value(zvalue, "%f", value);
            //SendAPI<float>(zapic, zapiv, zvalue, &value);
            sphactor_ask_api(this->actor, zconfig_value(zapic), zconfig_value(zapiv), ftoa(value) );
        }
    }
    else {
        if ( ImGui::InputFloat( name, &value, min, max) ) {
            zconfig_set_value(zvalue, "%f", value);
            //SendAPI<float>(zapic, zapiv, zvalue, &value);
            sphactor_ask_api(this->actor, zconfig_value(zapic), zconfig_value(zapiv), ftoa(value) );
        }
    }
    zconfig_t *help = zconfig_locate(data, "help");
    if (help)
    {
        char *helpv = zconfig_value(help);
        RenderTooltip(helpv);
    }
}

void
ActorContainer::RenderString(const char* name, zconfig_t *data) {
    int max = MAX_STR_DEFAULT;

    zconfig_t * zvalue = zconfig_locate(data, "value");
    zconfig_t * zapic = zconfig_locate(data, "api_call");
    zconfig_t * zapiv = zconfig_locate(data, "api_value");
    assert(zvalue);

    zconfig_t * zmax = zconfig_locate(data, "max");

    ReadInt( &max, zmax );

    char buf[MAX_STR_DEFAULT];
    const char* zvalueStr = zconfig_value(zvalue);
    strcpy(buf, zvalueStr);
    char *p = &buf[0];

    ImGui::SetNextItemWidth(200);
    if ( ImGui::InputText(name, buf, max) ) {
        zconfig_set_value(zvalue, "%s", buf);
        sphactor_ask_api(this->actor, zconfig_value(zapic), zconfig_value(zapiv), buf);
    }

    zconfig_t *help = zconfig_locate(data, "help");
    if (help)
    {
        char *helpv = zconfig_value(help);
        RenderTooltip(helpv);
    }
}

void
ActorContainer::RenderMultilineString(const char* name, zconfig_t *data) {
    int max = MAX_STR_DEFAULT;

    zconfig_t * zvalue = zconfig_locate(data, "value");
    zconfig_t * zapic = zconfig_locate(data, "api_call");
    zconfig_t * zapiv = zconfig_locate(data, "api_value");
    assert(zvalue);

    zconfig_t * zmax = zconfig_locate(data, "max");

    ReadInt( &max, zmax );

    char buf[MAX_STR_DEFAULT];
    const char* zvalueStr = zconfig_value(zvalue);
    strcpy(buf, zvalueStr);
    s_replace_char(buf, ',', '\n');  // replace comma with newline if needed
    char *p = &buf[0];

    ImGui::SetNextItemWidth(200);
    ImGui::InputTextMultiline("##source", p, max, ImVec2(0, ImGui::GetTextLineHeight() * 8), ImGuiInputTextFlags_EnterReturnsTrue);
    if ( ImGui::IsItemEdited() || ImGui::IsItemDeactivatedAfterEdit())
    {
        char *sendbuf = strdup(buf);
        s_replace_char(sendbuf, '\n', ','); // We use ',' instead of newline since we can't save newlines in the stage save file
        zconfig_set_value(zvalue, "%s", sendbuf);
        sphactor_ask_api(this->actor, zconfig_value(zapic), zconfig_value(zapiv), sendbuf);
        zstr_free(&sendbuf);
    }

    zconfig_t *help = zconfig_locate(data, "help");
    if (help)
    {
        char *helpv = zconfig_value(help);
        RenderTooltip(helpv);
    }
}

void
ActorContainer::RenderTrigger(const char* name, zconfig_t *data) {
    zconfig_t * zapic = zconfig_locate(data, "api_call");

    ImGui::SetNextItemWidth(200);
    if ( ImGui::Button(name) ) {
        sphactor_ask_api(this->actor, zconfig_value(zapic), "", "" );
    }
    zconfig_t *help = zconfig_locate(data, "help");
    if (help)
    {
        char *helpv = zconfig_value(help);
        RenderTooltip(helpv);
    }
}

void
ActorContainer::ReadBool( bool *value, zconfig_t * data) {
    if ( data != NULL ) {
        *value = streq( zconfig_value(data), "True");
    }
}

void
ActorContainer::ReadInt( int *value, zconfig_t * data) {
    if ( data != NULL ) {
        *value = atoi(zconfig_value(data));
    }
}

void
ActorContainer::ReadFloat( float *value, zconfig_t * data) {
    if ( data != NULL ) {
        *value = atof(zconfig_value(data));
    }
}

void
ActorContainer::DestroyActor() {
    if ( actor )
        sphactor_destroy(&actor);
}

void
ActorContainer::DeleteConnection(const Connection& connection)
{
    for (auto it = connections.begin(); it != connections.end(); ++it)
    {
        if (connection == *it)
        {
            connections.erase(it);
            break;
        }
    }
}

//TODO: Add custom report data to this?
void
ActorContainer::SerializeActorData(zconfig_t *section) {
    zconfig_t *xpos = zconfig_new("xpos", section);
    zconfig_set_value(xpos, "%f", pos.x);

    zconfig_t *ypos = zconfig_new("ypos", section);
    zconfig_set_value(ypos, "%f", pos.y);

    // Parse current state of data capabilities
    if ( this->capabilities ) {
        zconfig_t *root = zconfig_locate(this->capabilities, "capabilities");
        if ( root ) {
            zconfig_t *data = zconfig_locate(root, "data");
            while ( data ) {
                zconfig_t *name = zconfig_locate(data,"name");
                zconfig_t *value = zconfig_locate(data,"value");

                if (value) // only store if there's a value
                {
                    char *nameStr = zconfig_value(name);
                    char *valueStr = zconfig_value(value);

                    zconfig_t *stored = zconfig_new(nameStr, section);
                    zconfig_set_value(stored, "%s", valueStr);
                }

                data = zconfig_next(data);
            }
        }
    }
}

void
ActorContainer::DeserializeActorData( ImVector<char*> *args, ImVector<char*>::iterator it) {
    char* xpos = *it;
    it++;
    char* ypos = *it;
    it++;

    if ( it != args->end()) {
        if ( this->capabilities ) {
            zconfig_t *root = zconfig_locate(this->capabilities, "capabilities");
            if ( root ) {
                zconfig_t *data = zconfig_locate(root, "data");
                while ( data && it != args->end() ) {
                    zconfig_t *value = zconfig_locate(data,"value");
                    if (value)
                    {
                        char* valueStr = *it;
                        zconfig_set_value(value, "%s", valueStr);

                        HandleAPICalls(data);
                        it++;
                    }
                    data = zconfig_next(data);
                }
            }
        }
    }

    pos.x = atof(xpos);
    pos.y = atof(ypos);

    free(xpos);
    free(ypos);
}

void
ActorContainer::OpenTextEditor(const char *filepath)
{
    gzb::TextEditorWindow *txtwin = NULL;
    for ( auto w : gzb::App::getApp().text_editors )
    {
        if ( w->associated_file == filepath )
        {
            txtwin = w;
        }
    }
    if (txtwin == NULL)
    {
        txtwin = new gzb::TextEditorWindow(filepath);
        txtwin->showing = true;
        gzb::App::getApp().text_editors.push_back(txtwin);
    }
    else
    {
        txtwin->showing = true;
        ImGuiWindow *win = ImGui::FindWindowByName(txtwin->window_name.c_str());
        ImGui::FocusWindow(win);
    }
}

};// namespace

