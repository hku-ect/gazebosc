#include <string>
#include "SDL.h"
#include "StageWindow.hpp"
#include "App.hpp"
#include "glm/glm/common.hpp"
#include "helpers.h"
#include "ext/ImFileDialog/ImFileDialog.h"

namespace gzb {

inline ImU32 LerpImU32( ImU32 c1, ImU32 c2, int index, int total, float offset, bool doubleSided )
{
    float t = (float)index / total;
    t = glm::max(glm::min(t + offset, 1.0f), 0.0f);

    if ( doubleSided && t < .5f ) {
        ImU32 col;
        col = c1;
        c1 = c2;
        c2 = col;
    }

    unsigned char   a1 = (c1 >> 24) & 0xff;
    unsigned char   a2 = (c2 >> 24) & 0xff;
    unsigned char   r1 = (c1 >> 16) & 0xff;
    unsigned char   r2 = (c2 >> 16) & 0xff;
    unsigned char   g1 = (c1 >> 8) & 0xff;
    unsigned char   g2 = (c2 >> 8) & 0xff;
    unsigned char   b1 = c1 & 0xff;
    unsigned char   b2 = c2 & 0xff;

    return  (int) ((a2 - a1) * t + a1) << 24 |
           (int) ((r2 - r1) * t + r1) << 16 |
           (int) ((g2 - g1) * t + g1) << 8 |
           (int) ((b2 - b1) * t + b1);
}

StageWindow::StageWindow()
{
    window_name = "Stage Window";
    // very hackish still
    // enforcable maximum actor counts
    max_actors_by_type.insert(std::make_pair("NatNet", 1));
    max_actors_by_type.insert(std::make_pair("OpenVR", 1));
}

StageWindow::~StageWindow()
{
}

void StageWindow::Init()
{
    // initialise an empty stage with tmp working dir
    assert(stage == NULL);

    // set a temporary random working dir for our stage
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    char tmpdir[12] = "_gzs_xxxxxx";
    for (int i=5;i<12;i++)
    {
        int key = rand() % (int)(sizeof(charset)-1);
        tmpdir[i] = charset[key];
    }
    tmpdir[11] = 0; // null termination
    std::filesystem::path tmppath(GZB_GLOBAL.TMPPATH);
    tmppath.append(tmpdir);
    std::error_code ec;
    if ( ! std::filesystem::create_directory(tmppath, ec) )
    {
        // TODO: what to do if creating the dir fails?
        zsys_error("Creating tmp dir %s failed, this might mean trouble!!!", tmppath.string().c_str() );
    }
    else
    {
        std::filesystem::current_path(tmppath);
        zsys_info("Temporary stage dir is now at %s", tmppath.string().c_str());
    }
    // clear active file as it needs saving to become a file first
    editing_file = "";
    editing_path = "";
}

void StageWindow::Clear()
{
    undoStack = std::stack<UndoData>();
    redoStack = std::stack<UndoData>();

    //delete all connections
    for (auto it = actors.begin(); it != actors.end();)
    {
        ActorContainer* gActor = *it;

        for (auto& connection : gActor->connections)
        {
            //delete them once
            if (connection.output_node == gActor) {
                ((ActorContainer*) connection.input_node)->DeleteConnection(connection);
            }
        }
        gActor->connections.clear();
        it++;
    }

    sph_stage_destroy(&stage);
    // remove working dir if it's a temporary path
    std::error_code ec; // no exception
    std::filesystem::path cwd = std::filesystem::current_path(ec);
#ifdef PYTHON3_FOUND
    python_remove_path(cwd.string().c_str());
#endif
    // temporary change the working dir otherwise we cannot delete it on windows, Load or Init will reset it
    std::filesystem::current_path(GZB_GLOBAL.TMPPATH);

    std::filesystem::path tmppath(GZB_GLOBAL.TMPPATH);
    tmppath.append("_gzs_");
    if ( cwd.string().rfind(tmppath.string(), 0) == 0 )
    {
        try
        {
            std::filesystem::remove_all(cwd);
        }
        catch (std::exception& e)
        {
            zsys_error("failed to remove working dir: %s", e.what());
        }
    }
    assert(stage == NULL);

    //delete all actors
    for (auto it = actors.begin(); it != actors.end();)
    {
        ActorContainer* actor = *it;
        // sphactor is already destroyed by stage
        delete actor;
        it = actors.erase(it);
    }
    // clear active file and set new path
    editing_file = "";
    editing_path = "";
}

bool StageWindow::Load( const char* configFile )
{
    undoStack = std::stack<UndoData>();
    redoStack = std::stack<UndoData>();

    // Clear current stage
    // TODO: Maybe ask if people want to save first?
    if (stage)
        Clear();

    stage = sph_stage_load(configFile);
    if ( stage == NULL )
        return false;
#ifdef PYTHON3_FOUND
    std::error_code ec; // no exception
    std::filesystem::path cwd = std::filesystem::current_path(ec);
    python_add_path(cwd.string().c_str());
#endif

    // clear active file as it needs saving to become a file first
    editing_file = std::string(configFile);
    editing_path = std::filesystem::path( std::filesystem::current_path() /= std::string(configFile)).string();

    // Create a container for every actor
    const zhash_t *stage_actors = sph_stage_actors(stage);
    sphactor_t *actor = (sphactor_t *)zhash_first( (zhash_t *)stage_actors ); //zconfig_locate(configActors, "actor");
    while( actor != NULL )
    {
        const char *uuidStr = zuuid_str(sphactor_ask_uuid(actor));
        const char *typeStr = sphactor_ask_actor_type(actor);
        const char *endpointStr = sphactor_ask_endpoint(actor);

        ActorContainer *gActor = new ActorContainer( actor ); //CreateFromType(typeStr, uuidStr);

        actors.push_back(gActor);
        actor = (sphactor_t *)zhash_next((zhash_t *)stage_actors);
    }
    // loop actor containers to handle all connections
    auto it = actors.begin();//(sphactor_t *)zhash_first( (zhash_t *)stage_actors ); //zconfig_locate(configActors, "actor");
    while( it != actors.end() )
    {
        ActorContainer *gActor = *it;
        // get actor's connections
        zlist_t *conn = sphactor_connections( gActor->actor );
        for ( char *connection = (char *)zlist_first(conn); connection != nullptr; connection = (char *)zlist_next(conn))
        {
            // find connect actor container
            ActorContainer *peer_actor_container = Find(connection);
            if (peer_actor_container != nullptr)
            {
                Connection new_connection;
                new_connection.input_node = gActor;
                new_connection.input_slot = gActor->input_slots[0].title;
                new_connection.output_node = peer_actor_container;
                new_connection.output_slot = peer_actor_container->output_slots[0].title;
                ((ActorContainer*) new_connection.input_node)->connections.push_back(new_connection);
                ((ActorContainer*) new_connection.output_node)->connections.push_back(new_connection);
            }
        }
        ++it;
    }

    return stage != NULL;
}

bool StageWindow::Save( const char* configFile )
{
    if ( actors.size() == 0 ) return false;

    // sync positions
    auto it = actors.begin();
    while( it != actors.end() )
    {
        ActorContainer *gActor = *it;
        sphactor_set_position(gActor->actor, gActor->pos.x, gActor->pos.y);
        ++it;
    }

    int rc = sph_stage_save_as( stage, configFile);
    return rc == 0;
}

int StageWindow::RenderMenuBar()
{
    static char* configFile = new char[64] { 0x0 };
    static int keyFocus = 0;
    MenuAction action = MenuAction_None;

    if ( keyFocus > 0 ) keyFocus--;

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
                TextEditorWindow *te = new gzb::TextEditorWindow();
                gzb::App::getApp().text_editors.push_back(te);
                te->showing = true;
            }
            for (auto w : gzb::App::getApp().text_editors )
            {
                std::string title = w->window_name + " " + ICON_FA_EYE;
                if ( ImGui::MenuItem(w->window_name.c_str(), NULL, w->showing, true ) )
                    w->showing = !w->showing;
            }
            ImGui::EndMenu();
        }
        if ( ImGui::MenuItem(ICON_FA_BARS " Toggle Log Console") ) {
            gzb::App::getApp().log_win.showing = !gzb::App::getApp().log_win.showing;
        }
#ifdef PYTHON3_FOUND
        if ( ImGui::MenuItem(ICON_FA_TERMINAL " Toggle Python Console") ) {
            gzb::App::getApp().py_win.showing = !gzb::App::getApp().py_win.showing;
        }
#endif
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


    if ( streq( editing_file.c_str(), "" ) ) {
        ImGui::TextColored( ImVec4(.7,.9,.7,1), ICON_FA_EDIT ": *Unsaved Stage*");
    }
    else {
        ImGui::TextColored( ImVec4(.7,.9,.7,1), ICON_FA_EDIT ": %s", editing_file.c_str());
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
    if ( action == MenuAction_Load)
    {
        ifd::FileDialog::Instance().Open(window_name + "LoadStageDialog", "Load stage from file", "Gazebo Stage(*.gzs){.gzs},.*");
        keyFocus = 2;
    }
    else if ( action == MenuAction_Save ) {
        if ( streq( editing_file.c_str(), "" ) ) {
            ifd::FileDialog::Instance().Save(window_name + "SaveStageDialog", "Save stage to file", "Gazebo Stage(*.gzs){.gzs},.*");
        }
        else {
            Save(editing_path.c_str());
        }
    }
    else if ( action == MenuAction_SaveAs ) {
        ifd::FileDialog::Instance().Save(window_name + "SaveStageDialog", "Save stage to file", "Gazebo Stage(*.gzs){.gzs},.*");
    }
    else if ( action == MenuAction_Clear ) {
        Clear();
        Init();
    }
    else if ( action == MenuAction_Exit ) {
        return -1;
    }

    if (ifd::FileDialog::Instance().IsDone(window_name + "LoadStageDialog")) {
        if (ifd::FileDialog::Instance().HasResult()) {
            std::string res = ifd::FileDialog::Instance().GetResult().u8string();
            if (Load(res.c_str())) {
                editing_file = ifd::FileDialog::Instance().GetResult().filename().u8string();
                editing_path = res;
                moveCwdIfNeeded();
            }
        }
        ifd::FileDialog::Instance().Close();
    }

    if (ifd::FileDialog::Instance().IsDone(window_name + "SaveStageDialog"))
    {
        if (ifd::FileDialog::Instance().HasResult())
        {
            std::string res = ifd::FileDialog::Instance().GetResult().u8string();
            if ( !Save(res.c_str()) )
            {
                editing_file = "";
                editing_path = "";
                zsys_error("Saving file failed!");
            }
            else
            {
                editing_file = ifd::FileDialog::Instance().GetResult().filename().u8string();
                editing_path = res;
                moveCwdIfNeeded();
            }
        }
        ifd::FileDialog::Instance().Close();
    }

    return 0;
}

int StageWindow::UpdateActors()
{
    static std::vector<ActorContainer*> selectedActors;
    static std::vector<char *> actorClipboardType;
    static std::vector<char *> actorClipboardCapabilities;
    static std::vector<ImVec2> actorClipboardPositions;
    static bool D_PRESSED = false;

    int numKeys;
    byte * keyState = (byte*)SDL_GetKeyboardState(&numKeys);

    int rc = 0;
    static ImNodes::Ez::Context* context = ImNodes::Ez::CreateContext();
    IM_UNUSED(context);

    ImGui::SetNextWindowPos(ImVec2(0,0));

    ImGuiIO &io = ImGui::GetIO();
    bool copy = ImGui::IsKeyPressed(ImGuiKey_C) && (io.KeySuper || io.KeyCtrl);
    bool paste = ImGui::IsKeyPressed(ImGuiKey_V) && (io.KeySuper || io.KeyCtrl);
    bool dup = ( keyState[SDL_SCANCODE_D] == 1 && !D_PRESSED) && (io.KeySuper || io.KeyCtrl);
    bool undo = ImGui::IsKeyPressed(ImGuiKey_Z) && (io.KeySuper || io.KeyCtrl);
    bool redo = ImGui::IsKeyPressed(ImGuiKey_Y) && (io.KeySuper || io.KeyCtrl);
    bool del = ImGui::IsKeyPressed(ImGuiKey_Delete) || (io.KeySuper && ImGui::IsKeyPressed(ImGuiKey_Backspace));

    D_PRESSED = keyState[SDL_SCANCODE_D] == 1;

    if ( undo ) {
        if ( undoStack.size() > 0 ) {
            UndoData top = undoStack.top();
            performUndo( top );
            swapUndoType(&top);
            redoStack.push(UndoData(&top));
            undoStack.pop();
        }
    }
    else if ( redo ) {
        if ( redoStack.size() > 0 ) {
            UndoData top = redoStack.top();
            performUndo( top );
            swapUndoType(&top);
            undoStack.push(UndoData(&top));
            redoStack.pop();
        }
    }
    else if (del) {
        // zsys_info("DEL");
        // Loop and delete connections of actors connected to us
        for (auto it = std::begin(selectedActors); it != std::end(selectedActors); it++)
        {
            ActorContainer* actor = *it;
            for (auto& connection : actor->connections) {
                if (connection.output_node == actor) {
                    ((ActorContainer*)connection.input_node)->DeleteConnection(connection);
                }
                else {
                    ((ActorContainer*)connection.output_node)->DeleteConnection(connection);
                }
                RegisterDisconnectAction((ActorContainer*)connection.input_node, (ActorContainer*)connection.output_node, connection.input_slot, connection.output_slot);
            }

            RegisterDeleteAction(actor);

            // Delete all our connections separately
            actor->connections.clear();
            sph_stage_remove_actor(stage, zuuid_str(sphactor_ask_uuid(actor->actor)));

            // find and remove the actor from the actors list
            for (int i = 0; i < actors.size(); ++i )
            {
                if (actors.at(i) == actor) actors.erase(actors.begin() + i--);
            }
            delete actor;
        }
    }
    else if ( selectedActors.size() > 0 && copy && !ImGui::IsAnyItemActive() )
    {
        // copy actor data to clipboard char *'s
        // zsys_info("COPY" );
        for( int i = 0; i < actorClipboardType.size(); ++i ) {
            free(actorClipboardType.at(i));
            free(actorClipboardCapabilities.at(i));
        }
        actorClipboardType.clear();
        actorClipboardCapabilities.clear();
        actorClipboardPositions.clear();

        for( int i = 0; i < selectedActors.size(); ++i ) {
            ActorContainer * selectedActor = selectedActors.at(i);

            actorClipboardCapabilities.push_back(zconfig_str_save(selectedActor->capabilities));

            int length = strlen(selectedActor->title);
            actorClipboardType.push_back(new char[length+1]);
            strncpy(actorClipboardType.back(), selectedActor->title, length);
            actorClipboardType.back()[length] = '\0';

            actorClipboardPositions.push_back(ImVec2(selectedActor->pos - io.MousePos));
        }
    }
    else if ( actorClipboardType.size() > 0 && paste && !ImGui::IsAnyItemActive() ) {
        // create actor(s) from clipboard
        // clear clipboard
        for( int i = 0; i < actorClipboardType.size(); ++i ) {
            char * type = actorClipboardType.at(i);
            char * capabilities = actorClipboardCapabilities.at(i);

            // zsys_info("PASTE: %s", type);

            int max = -1;
            if ( max_actors_by_type.find(type) != max_actors_by_type.end() )
                max = max_actors_by_type.at(type);

            if ( CountActorsOfType(type) < max || max == -1) {
                ActorContainer *actor = CreateFromType(type, nullptr);
                actor->SetCapabilities(capabilities);
                actor->pos = io.MousePos + actorClipboardPositions.at(i);
                actors.push_back(actor);

                RegisterCreateAction(actor);
            }
        }

        // TODO: figure out when we need to clear this (if ever)
        /*
        free(actorClipboardType);
        free(actorClipboardCapabilities);
        actorClipboardType = nullptr;
        actorClipboardCapabilities = nullptr;
        */
    }
    else if ( selectedActors.size() > 0 && dup && !ImGui::IsAnyItemActive() ) {
        // zsys_info("DUP");
        std::vector<char *> dupClipboardType;
        std::vector<char *> dupClipboardCapabilities;
        std::vector<ImVec2> dupClipboardPositions;

        ImVec2 center;

        for( int i = 0; i < selectedActors.size(); ++i ) {
            ActorContainer * selectedActor = selectedActors.at(i);

            dupClipboardCapabilities.push_back(zconfig_str_save(selectedActor->capabilities));

            int length = strlen(selectedActor->title);
            dupClipboardType.push_back(new char[length+1]);
            strncpy(dupClipboardType.back(), selectedActor->title, length);
            dupClipboardType.back()[length] = '\0';

            center += selectedActor->pos;
        }

        center /= selectedActors.size();
        for( int i = 0; i < selectedActors.size(); ++i ) {
            ActorContainer * selectedActor = selectedActors.at(i);
            dupClipboardPositions.push_back(ImVec2(selectedActor->pos - center));
        }

        selectedActors.clear();

        for( int i = 0; i < dupClipboardType.size(); ++i ) {
            char * type = dupClipboardType.at(i);
            char * capabilities = dupClipboardCapabilities.at(i);

            // zsys_info("PASTE: %s", type);

            int max = -1;
            if ( max_actors_by_type.find(type) != max_actors_by_type.end() )
                max = max_actors_by_type.at(type);

            if ( CountActorsOfType(type) < max || max == -1) {
                ActorContainer *actor = CreateFromType(type, nullptr);
                actor->SetCapabilities(capabilities);
                actor->pos = io.MousePos + dupClipboardPositions.at(i) + ImVec2(10,10);
                actors.push_back(actor);

                // TODO: SELECT DUPLICATED ACTORS!?
                // selectedActors.push_back(actor);

                RegisterCreateAction(actor);
            }
        }
    }

    selectedActors.clear();
    if (ImGui::Begin("ImNodes", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_MenuBar ))
    {
        rc = RenderMenuBar();

        // We probably need to keep some state, like positions of nodes/slots for rendering connections.
        ImNodes::Ez::BeginCanvas();

        for (auto it = std::begin(actors); it != std::end(actors); it++)
        {
            ActorContainer* actor = *it;

            if ( abs(actor->pos.x) > 100000000 || abs(actor->pos.y) > 100000000 ) {
                actor->pos = ImGui::GetMousePos();
            }

            // Start rendering node
            if (ImNodes::Ez::BeginNode(actor, actor->title, &actor->pos, &actor->selected))
            {
                // Render input nodes first (order is important)
                ImNodes::Ez::InputSlots(actor->input_slots.data(), actor->input_slots.size());

                // Custom node content may go here
                actor->Render();

                // Render output slots second (order is important)
                ImNodes::Ez::OutputSlots(actor->output_slots.data(), actor->output_slots.size());

                // Store new connections when they are created
                Connection new_connection;
                if (ImNodes::GetNewConnection(&new_connection.input_node, &new_connection.input_slot,
                                              &new_connection.output_node, &new_connection.output_slot))
                {
                    assert(new_connection.input_node);
                    assert(new_connection.output_node);
                    ((ActorContainer*) new_connection.input_node)->connections.push_back(new_connection);
                    ((ActorContainer*) new_connection.output_node)->connections.push_back(new_connection);
                    sphactor_ask_connect( ((ActorContainer*) new_connection.input_node)->actor,
                                         sphactor_ask_endpoint( ((ActorContainer*) new_connection.output_node)->actor ) );

                    RegisterConnectAction((ActorContainer*)new_connection.input_node, (ActorContainer*)new_connection.output_node, new_connection.input_slot, new_connection.output_slot);
                }

                // Render output connections of this actor
                for (const Connection& connection : actor->connections)
                {
                    // Node contains all it's connections (both from output and to input slots). This means that multiple
                    // nodes will have same connection. We render only output connections and ensure that each connection
                    // will be rendered once.
                    if (connection.output_node != actor)
                        continue;

                    ImDrawList * draw_list = ImGui::GetWindowDrawList();
                    int oldId = draw_list->_VtxCurrentIdx;

                    if (!ImNodes::Connection(connection.input_node, connection.input_slot, connection.output_node,
                                             connection.output_slot))
                    {
                        assert(connection.input_node);
                        assert(connection.output_node);
                        sphactor_ask_disconnect( ((ActorContainer*) connection.input_node)->actor,
                                                sphactor_ask_endpoint( ((ActorContainer*) connection.output_node)->actor ) );
                        // Remove deleted connections
                        ((ActorContainer*) connection.input_node)->DeleteConnection(connection);
                        ((ActorContainer*) connection.output_node)->DeleteConnection(connection);

                        RegisterDisconnectAction((ActorContainer*)connection.input_node, (ActorContainer*)connection.output_node, connection.input_slot, connection.output_slot);
                    }
                    else
                    {
                        // Animate the bezier vertex colors for recently active connections
                        int64_t diff = zclock_mono() - ((ActorContainer*) connection.output_node)->lastActive;
                        ImNodes::CanvasState *canvas = ImNodes::GetCurrentCanvas();
                        if ( diff < 500 ) {
                            float offset = ( diff % 500 - 250 ) * .004f;
                            int newId = draw_list->_VtxCurrentIdx;
                            int totalCount = (newId - oldId);

                            // We're assuming it's always a 2-vertex-per-point setup (thick/textured)
                            if (totalCount % 2 == 0) {
                                int left = totalCount;
                                ImDrawVert *p = draw_list->_VtxWritePtr - totalCount;
                                while (left > 0) {
                                    int index = (totalCount - left) * .5f;
                                    // TODO: Determine what colors we want to render...
                                    ImU32 col = LerpImU32(ImColor(0, 255, 0),
                                                          canvas->Colors[ImNodes::ColConnection], index,
                                                          totalCount / 2, offset, false);

                                    p->col = col;
                                    (p + 1)->col = col;

                                    left -= 2;
                                    p += 2;
                                }
                            }
                        }
                    }
                }
            }
            // Node rendering is done. This call will render node background based on size of content inside node.
            ImNodes::Ez::EndNode();

            if ( actor->selected) {
                selectedActors.push_back(actor);
            }
        }

        if (ImGui::IsMouseReleased(1) && ImGui::IsWindowHovered() && !ImGui::IsMouseDragging(1))
        {
            ImGui::FocusWindow(ImGui::GetCurrentWindow());
            ImGui::OpenPopup("NodesContextMenu");
        }

        if (ImGui::BeginPopup("NodesContextMenu"))
        {
            //TODO: Fetch updated available nodes?

            zlist_t *actor_types = zhash_keys(sphactor_get_registered());
            assert(actor_types);
            char *desc = (char *)zlist_first(actor_types);
            while(desc != NULL)
            {
                if (ImGui::MenuItem(desc))
                {
                    if ( max_actors_by_type.find(desc) != max_actors_by_type.end() ) {
                        int max = max_actors_by_type.at(desc);
                        if ( CountActorsOfType(desc) < max ) {
                            ActorContainer *actor = CreateFromType(desc, nullptr);
                            actors.push_back(actor);
                            ImNodes::AutoPositionNode(actors.back());
                            RegisterCreateAction(actor);
                        }
                    }
                    else {
                        ActorContainer *actor = CreateFromType(desc, nullptr);
                        actors.push_back(actor);
                        ImNodes::AutoPositionNode(actors.back());
                        RegisterCreateAction(actor);
                    }
                }
                desc = (char *)zlist_next(actor_types);
            }

            ImGui::Separator();
            if (ImGui::MenuItem("Reset Zoom"))
                ImNodes::GetCurrentCanvas()->Zoom = 1;

            if (ImGui::IsAnyMouseDown() && !ImGui::IsWindowHovered())
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        ImNodes::Ez::EndCanvas();
    }
    ImGui::End();

    return rc;
}

ActorContainer *StageWindow::Find(const char *endpoint)
{
    for (auto it = actors.begin(); it != actors.end();)
    {
        ActorContainer* actor = *it;
        if ( streq( sphactor_ask_endpoint(actor->actor), endpoint)) {
            return actor;
        }

        it++;
    }
    return nullptr;
}

ActorContainer * StageWindow::CreateFromType( const char* typeStr, const char* uuidStr )
{
    zuuid_t *uuid = zuuid_new();
    if ( uuidStr != NULL )
        zuuid_set_str(uuid, uuidStr);

    sphactor_t *actor = sphactor_new_by_type(typeStr, uuidStr, uuid);
    sphactor_ask_set_actor_type(actor, typeStr);
    if ( stage == nullptr )
        stage = sph_stage_new("New Stage");
    sph_stage_add_actor(stage, actor);
    return new ActorContainer(actor);
}

int StageWindow::CountActorsOfType( const char* type )
{
    int count = 0;
    for (auto it = actors.begin(); it != actors.end(); it++)
    {
        ActorContainer* actor = *it;
        if ( streq( actor->title, type ) ) {
            count++;
        }
    }
    return count;
}

// undo stuff
void StageWindow::performUndo(UndoData &undo)
{
    switch( undo.type) {
    case UndoAction_Delete: {
        zsys_info("UNDO DELETE");
        sphactor_t * sphactor_actor = sphactor_load(undo.sphactor_config);
        sph_stage_add_actor(stage, sphactor_actor);

        ActorContainer *actor = new ActorContainer( sphactor_actor );
        //zsys_info("UUID: %s", zuuid_str(sphactor_ask_uuid(sphactor_actor)));
        actor->pos = undo.position;
        actors.push_back(actor);
        break;
    }
    case UndoAction_Create: {
        zsys_info("UNDO CREATE");
        for( auto it = actors.begin(); it != actors.end(); it++) {
            ActorContainer* actor = *it;
            if ( streq( zuuid_str(sphactor_ask_uuid(actor->actor)), undo.uuid)) {
                // Delete all our connections separately
                actor->connections.clear();
                sph_stage_remove_actor(stage, zuuid_str(sphactor_ask_uuid(actor->actor)));

                delete actor;
                actors.erase(it);
                break;
            }
        }
        break;
    }
    case UndoAction_Connect: {
        zsys_info("UNDO CONNECT");
        ActorContainer *out = nullptr, *in = nullptr;
        for( auto it = actors.begin(); it != actors.end(); it++) {
            ActorContainer* actor = *it;
            if ( streq( zuuid_str(sphactor_ask_uuid(actor->actor)), undo.uuid)) {
                in = *it;
            }
            else if ( streq( sphactor_ask_endpoint(actor->actor), undo.endpoint)) {
                out = *it;
            }
            if (out && in) break;
        }

        if ( !out || !in ) {
            zsys_info("COULD NOT FIND ONE OF THE NODES");
            break;
        }

        // find and delete connections
        for( auto it = in->connections.begin(); it != in->connections.end(); it++ ) {
            ActorContainer *actor = (ActorContainer*)(*it).output_node;
            if ( (*it).output_node == out) {
                in->connections.erase(it);
                break;
            }
        }
        for( auto it = out->connections.begin(); it != out->connections.end(); it++ ) {
            ActorContainer *actor = (ActorContainer*)(*it).input_node;
            if ( (*it).input_node == in) {
                out->connections.erase(it);
                break;
            }
        }

        // disconnect
        sphactor_ask_disconnect(in->actor, undo.endpoint);

        break;
    }
    case UndoAction_Disconnect: {
        zsys_info("UNDO DISCONNECT");
        ActorContainer *out = nullptr, *in = nullptr;
        for( auto it = actors.begin(); it != actors.end(); it++) {
            ActorContainer* actor = *it;
            if ( streq( zuuid_str(sphactor_ask_uuid(actor->actor)), undo.uuid)) {
                in = *it;
            }
            else if ( streq( sphactor_ask_endpoint(actor->actor), undo.endpoint)) {
                out = *it;
            }
            if (out && in) break;
        }

        if ( !out || !in ) {
            zsys_info("COULD NOT FIND ONE OF THE NODES");
            break;
        }

        //rebuild connection
        Connection new_connection;
        new_connection.input_slot = undo.input_slot;
        new_connection.output_slot = undo.output_slot;
        new_connection.input_node = in;
        new_connection.output_node = out;

        in->connections.push_back(new_connection);
        out->connections.push_back(new_connection);

        //connect
        sphactor_ask_connect(in->actor, undo.endpoint);

        break;
    }
    default: {
        break;
    }
    }
}

void StageWindow::RegisterCreateAction( ActorContainer * actor )
{
    if ( redoStack.size() > 0 ) {
        redoStack = std::stack<UndoData>();
    }

    UndoData undo;
    undo.type = UndoAction_Create;
    undo.title = strdup(actor->title);
    undo.sphactor_config = sphactor_save(actor->actor, nullptr);
    undo.uuid = strdup(zuuid_str(sphactor_ask_uuid(actor->actor)));
    zsys_info("UUID: %s", undo.uuid);
    ImGuiIO& io = ImGui::GetIO();
    if (actor->pos.x == 0 && actor->pos.y == 0) {
        undo.position = ImVec2(io.MousePos.x, io.MousePos.y);
    }
    else {
        undo.position = actor->pos;
    }

    undoStack.push(undo);
}

void StageWindow::RegisterDeleteAction( ActorContainer * actor )
{
    if ( redoStack.size() > 0 ) {
        redoStack = std::stack<UndoData>();
    }

    UndoData undo;
    undo.type = UndoAction_Delete;
    undo.title = strdup(actor->title);
    undo.sphactor_config = sphactor_save(actor->actor, nullptr);
    undo.uuid = strdup(zuuid_str(sphactor_ask_uuid(actor->actor)));
    zsys_info("UUID: %s", undo.uuid);
    undo.position = actor->pos;
    undoStack.push(undo);
}

void StageWindow::RegisterConnectAction(ActorContainer * in, ActorContainer * out, const char * input_slot, const char * output_slot)
{
    if ( redoStack.size() > 0 ) {
        redoStack = std::stack<UndoData>();
    }

    UndoData undo;
    undo.type = UndoAction_Connect;
    undo.uuid = strdup(zuuid_str(sphactor_ask_uuid(in->actor)));
    undo.endpoint = strdup(sphactor_ask_endpoint(out->actor));
    undo.input_slot = strdup(input_slot);
    undo.output_slot = strdup(output_slot);
    undoStack.push(undo);
}

void StageWindow::RegisterDisconnectAction(ActorContainer * in, ActorContainer * out, const char * input_slot, const char * output_slot)
{
    if ( redoStack.size() > 0 ) {
        redoStack = std::stack<UndoData>();
    }

    UndoData undo;
    undo.type = UndoAction_Disconnect;
    undo.uuid = strdup(zuuid_str(sphactor_ask_uuid(in->actor)));
    undo.endpoint = strdup(sphactor_ask_endpoint(out->actor));
    undo.input_slot = strdup(input_slot);
    undo.output_slot = strdup(output_slot);
    undoStack.push(undo);
}

void StageWindow::swapUndoType(UndoData * undo)
{
    switch(undo->type) {
    case UndoAction_Connect: {
        undo->type = UndoAction_Disconnect;
        break;
    }
    case UndoAction_Disconnect: {
        undo->type = UndoAction_Connect;
        break;
    }
    case UndoAction_Create: {
        undo->type = UndoAction_Delete;
        break;
    }
    case UndoAction_Delete: {
        undo->type = UndoAction_Create;
        break;
    }
    }
}

void StageWindow::moveCwdIfNeeded()
{
    // check if we are still in the working dir or if we should move to the new dir
    char cwd[PATH_MAX];
    getcwd(cwd, PATH_MAX);
    // editingPath not starting with cwd means we need to move to the new wd
    if (editing_path.rfind(cwd, 0) != 0) {
        // we're not in the current working dir! move files to the new wd
        // moving if cwd was tmp dir (name contains _gzs_)
        std::string cwds = std::string(cwd);
        std::filesystem::path newcwds = std::filesystem::path(editing_path);
        std::filesystem::path tmppath(GZB_GLOBAL.TMPPATH);
        tmppath.append("_gzs_");
        if (cwds.rfind(tmppath.string(), 0) == 0)
        {
            zsys_info("%s is a temporary stage dir. Moving files to %s", cwd, newcwds.parent_path().generic_u8string().c_str());
            // cwd is a tmp dir so we need to move files
            // copy and delete for now
            try
            {
                std::filesystem::copy(cwds, newcwds.parent_path(), std::filesystem::copy_options::overwrite_existing | std::filesystem::copy_options::recursive);
            }
            catch (std::exception& e)
            {
                zsys_error("Copy files to new working dir failed: %s", e.what());
            }
            try
            {
                std::filesystem::remove_all(cwds);
            }
            catch (std::exception& e)
            {
                zsys_error("Removing old tmpdir failed: %s", e.what());
            }
        }
        else
        {
            // we copy recursively
            // Recursively copies all files and folders from src to target and overwrites existing files in target.
            try
            {
                std::filesystem::copy(cwds, newcwds.parent_path(), std::filesystem::copy_options::overwrite_existing | std::filesystem::copy_options::recursive);
            }
            catch (std::exception& e)
            {
                zsys_error("Copy files to new working dir failed: %s", e.what());
            }
        }
        std::filesystem::current_path(newcwds.parent_path());
#ifdef PYTHON3_FOUND
        std::error_code ec; // no exception
        python_remove_path((const char *)cwd);
        python_add_path(newcwds.parent_path().string().c_str());
#endif
        zsys_info("Working dir set to %s", newcwds.parent_path().c_str());
    }
}

} //namespace
