//
// Copyright (c) 2017-2019 Rokas Kupstys.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#   define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include <vector>
#include <map>
#include <string>
#include <imgui.h>
#include "ImNodesEz.h"
#include "libsphactor.h"
#include "GActor.h"
#include "Actors/DefaultActors.h"
#ifdef PYTHON3_FOUND
#include "Actors/PythonActor.h"
#endif

//enum for menu actions
enum MenuAction
{
    MenuAction_Save = 0,
    MenuAction_Load,
    MenuAction_Clear,
    MenuAction_Exit,
    MenuAction_None
};

// List of actor types and constructors
//  Currently these are all internal dependencies, but we will need to create
//   a "generic node" class for types defined outside of this codebase.
ImVector<char*> actor_types;
std::map<std::string, GActor*(*)(const char*)> type_constructors;
std::vector<GActor*> actors;

bool Save( const char* configFile );
bool Load( const char* configFile );
void Clear();
GActor* Find( const char* endpoint );

typedef GActor* (*gactor_constructor_fn)(const char*);

void RegisterActor( const char* type, sphactor_handler_fn func, gactor_constructor_fn constructor ) {
    sphactor_register(type, func);
    type_constructors[type] = constructor;
}

void UpdateRegisteredActorsCache() {
    zlist_t* list = sphactor_get_registered();
    actor_types.clear();
    
    char* item = (char*)zlist_first(list);
    while( item ) {
        actor_types.push_back(item);
        item = (char*)zlist_next(list);
    }
}

GActor* CreateFromType( const char* typeStr, const char* uuidStr ) {
    auto typeIterator = type_constructors.find(typeStr);
    if ( typeIterator != type_constructors.end() ) {
        return type_constructors[typeStr](uuidStr);
    }
    else {
        //TODO: Implement base actor class
        zsys_error("Unknown actor type encountered: %s -> TODO: implement generic base actor", typeStr);
        return new RelayActor(uuidStr);
    }
}

void RegisterCPPActors() {
    
    // When implementing new actors, this is the only place you need to add them
    RegisterActor( "Log", GActor::_actor_handler, [](const char * uuid) -> GActor* { return new LogActor(uuid); });
    RegisterActor( "Midi", GActor::_actor_handler, [](const char * uuid) -> GActor* { return new MidiActor(uuid); });
    RegisterActor( "Count", GActor::_actor_handler, [](const char * uuid) -> GActor* { return new CountActor(uuid); });
    RegisterActor( "UDPSend", GActor::_actor_handler, [](const char * uuid) -> GActor* { return new UDPSendActor(uuid); });
    RegisterActor( "Pulse", GActor::_actor_handler, [](const char * uuid) -> GActor* { return new PulseActor(uuid); });
    RegisterActor( "Relay", GActor::_actor_handler, [](const char * uuid) -> GActor* { return new RelayActor(uuid); });
    RegisterActor( "OSCListener", GActor::_actor_handler, [](const char * uuid) -> GActor* { return new OSCListenerActor(uuid); });
    RegisterActor( "ManualPulse", GActor::_actor_handler, [](const char * uuid) -> GActor* { return new ManualPulse(uuid); });
#ifdef PYTHON3_FOUND
    python_init();
    RegisterActor( "Python", GActor::_actor_handler, [](const char * uuid) -> GActor* { return new PythonActor(uuid);});
#endif


    //TODO: Figure out when this needs to happen
    UpdateRegisteredActorsCache();
}

void ShowConfigWindow(bool * showLog) {
    static char* configFile = new char[64];
    
    //creates window
    ImGui::Begin("Stage Settings", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove );

    ImGui::Text("Save/load stage");

    ImGui::InputText("Filename", configFile, 128);
    
    if (ImGui::Button("Save")) {                           // Buttons return true when clicked (most widgets return true when edited/activated)
        Save(configFile);
    }
    ImGui::SameLine();
    ImGui::Spacing();
    ImGui::SameLine();
    if (ImGui::Button("Load")) {                           // Buttons return true when clicked (most widgets return true when edited/activated)
        Load(configFile);
    }
    
    ImGui::SameLine();
    ImGui::Spacing();
    ImGui::SameLine();
    if (ImGui::Button("Clear")) {                           // Buttons return true when clicked (most widgets return true when edited/activated)
        Clear();
    }
    
    ImGui::Checkbox("Show Log Window", showLog);
    
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::End();
}

void ShowLogWindow(ImGuiTextBuffer& buffer) {
    static bool ScrollToBottom = true;
    
    ImGui::PushID(123);
    
    ImGui::SetNextWindowSizeConstraints(ImVec2(100,100), ImVec2(1000,1000));
    ImGui::Begin("Console");
    
    if (ImGui::Button("Clear")) buffer.clear();
    ImGui::SameLine();
    if ( ImGui::Button("To Bottom") ) {
        ScrollToBottom = true;
    }
    
    ImGui::BeginChild("scrolling", ImVec2(0,0), false, ImGuiWindowFlags_HorizontalScrollbar);
    
    if ( !ScrollToBottom && ImGui::GetScrollY() == ImGui::GetScrollMaxY() ) {
        ScrollToBottom = true;
    }
    
    ImGui::TextUnformatted(buffer.begin());
    
    if (ScrollToBottom)
        ImGui::SetScrollHereY(1.0f);
    ScrollToBottom = false;

    ImGui::EndChild();
    
    ImGui::End();
    
    ImGui::PopID();
}

int RenderMenuBar( bool * showLog ) {
    static char* configFile = new char[64];
    static int keyFocus = 0;
    MenuAction action = MenuAction_None;
    
    if ( keyFocus > 0 ) keyFocus--;
    
    ImGui::BeginMainMenuBar();
    
    if ( ImGui::BeginMenu("File") ) {
        if ( ImGui::MenuItem("Save") ) {
            action = MenuAction_Save;
        }
        if ( ImGui::MenuItem("Load") ) {
            //TODO: support checking if changes were made
            action = MenuAction_Load;
        }
        ImGui::Separator();
        ImGui::Separator();
        if ( ImGui::MenuItem("Exit") ) {
            //TODO: support checking if changes were made
            action = MenuAction_Exit;
        }
        
        ImGui::EndMenu();
    }
    
    if ( ImGui::BeginMenu("Stage") ) {
        if ( ImGui::MenuItem("Clear") ) {
            action = MenuAction_Clear;
        }
        ImGui::EndMenu();
    }
    
    if ( ImGui::BeginMenu("Tools") ) {
        if ( ImGui::MenuItem("Toggle Console") ) {
            *showLog = !(*showLog);
        }
        ImGui::EndMenu();
    }
    
    //TODO: Display stage status (new, loaded, changed)
    ImGui::Separator();
    ImGui::Separator();
    
    if ( configFile[0] == 0 ) {
        ImGui::TextColored( ImVec4(.7,.9,.7,1), "   Editing: New Stage");
    }
    else {
        ImGui::TextColored( ImVec4(.7,.9,.7,1), "   Editing: %s", configFile);
    }
    
    ImGui::EndMainMenuBar();
    
    // Handle MenuActions
    if ( action == MenuAction_Load) {
        ImGui::OpenPopup("MenuAction_Load");
        keyFocus = 2;
    }
    else if ( action == MenuAction_Save ) {
        if ( configFile[0] == 0 ) {
            ImGui::OpenPopup("MenuAction_Save");
            keyFocus = 2;
        }
        else {
            Save(configFile);
            ImGui::CloseCurrentPopup();
        }
    }
    else if ( action == MenuAction_Clear ) {
        Clear();
        memset(configFile,0,64);
    }
    else if ( action == MenuAction_Exit ) {
        return -1;
    }
    
    if ( ImGui::BeginPopup("MenuAction_Load")) {
        ImGui::Text("Load stage");
        
        if ( keyFocus > 0 )
            ImGui::SetKeyboardFocusHere();
        ImGui::InputText("Filename", configFile, 128);
        
        if (ImGui::Button("Load")) {
            Load(configFile);
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    }
    
    if ( ImGui::BeginPopup("MenuAction_Save")) {
        ImGui::Text("Save stage");
        
        if ( keyFocus > 0 )
            ImGui::SetKeyboardFocusHere();
        ImGui::InputText("Filename", configFile, 128);
        
        if (ImGui::Button("Save")) {
            if ( !Save(configFile) ) {
                memset(configFile,0,64);
            }
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    }
    
    return 0;
}

int UpdateActors(float deltaTime, bool * showLog)
{
    int rc = 0;
    // Canvas must be created after ImGui initializes, because constructor accesses ImGui style to configure default colors.
    static ImNodes::CanvasState canvas{};
    
    //const ImGuiStyle& style = ImGui::GetStyle();
    
    ImGui::SetNextWindowPos(ImVec2(0,0));
    
    if (ImGui::Begin("ImNodes", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_MenuBar ))
    {
        rc = RenderMenuBar(showLog);
        
        // We probably need to keep some state, like positions of nodes/slots for rendering connections.
        ImNodes::BeginCanvas(&canvas);
        for (auto it = actors.begin(); it != actors.end();)
        {
            GActor* actor = *it;

            // Start rendering node
            if (ImNodes::Ez::BeginNode(actor, actor->title, &actor->pos, &actor->selected))
            {
                // Render input nodes first (order is important)
                ImNodes::Ez::InputSlots(actor->input_slots.data(), actor->input_slots.size());

                // Custom node content may go here
                actor->Render(deltaTime);

                // Render output slots second (order is important)
                ImNodes::Ez::OutputSlots(actor->output_slots.data(), actor->output_slots.size());

                // Store new connections when they are created
                Connection new_connection;
                if (ImNodes::GetNewConnection(&new_connection.input_node, &new_connection.input_slot,
                    &new_connection.output_node, &new_connection.output_slot))
                {
                    assert(new_connection.input_node);
                    assert(new_connection.output_node);
                    ((GActor*) new_connection.input_node)->connections.push_back(new_connection);
                    ((GActor*) new_connection.output_node)->connections.push_back(new_connection);
                    sphactor_ask_connect( ((GActor*) new_connection.input_node)->actor,
                                      sphactor_ask_endpoint( ((GActor*) new_connection.output_node)->actor ) );
                }

                // Render output connections of this actor
                for (const Connection& connection : actor->connections)
                {
                    // Node contains all it's connections (both from output and to input slots). This means that multiple
                    // nodes will have same connection. We render only output connections and ensure that each connection
                    // will be rendered once.
                    if (connection.output_node != actor)
                        continue;

                    if (!ImNodes::Connection(connection.input_node, connection.input_slot, connection.output_node,
                        connection.output_slot))
                    {
                        assert(connection.input_node);
                        assert(connection.output_node);
                        sphactor_ask_disconnect( ((GActor*) connection.input_node)->actor,
                                          sphactor_ask_endpoint( ((GActor*) connection.output_node)->actor ) );
                        // Remove deleted connections
                        ((GActor*) connection.input_node)->DeleteConnection(connection);
                        ((GActor*) connection.output_node)->DeleteConnection(connection);
                    }
                }
            }
            // Node rendering is done. This call will render node background based on size of content inside node.
            ImNodes::Ez::EndNode();
            
            bool del = ImGui::IsKeyPressedMap(ImGuiKey_Delete) || ( ImGui::IsKeyPressedMap(ImGuiKey_Backspace) && ImGui::GetActiveID() == 0 );
            if (actor->selected && del)
            {
                // Loop and delete connections of actors connected to us
                for (auto& connection : actor->connections)
                {
                    if (connection.output_node == actor) {
                        ((GActor*) connection.input_node)->DeleteConnection(connection);
                    }
                    else {
                        ((GActor*) connection.output_node)->DeleteConnection(connection);
                    }
                }
                // Delete all our connections separately
                actor->connections.clear();
                actor->DestroyActor();
                
                delete actor;
                it = actors.erase(it);
            }
            else
                ++it;
        }
        
        if (ImGui::IsMouseReleased(1) && ImGui::IsWindowHovered() && !ImGui::IsMouseDragging(1))
        {
            ImGui::FocusWindow(ImGui::GetCurrentWindow());
            ImGui::OpenPopup("NodesContextMenu");
        }
        
        if (ImGui::BeginPopup("NodesContextMenu"))
        {
            //TODO: Fetch updated available nodes?
            for (const auto desc : actor_types)
            {
                if (ImGui::MenuItem(desc))
                {
                    GActor* actor = CreateFromType(desc, nullptr);
                    actor->CreateActor();
                    actors.push_back(actor);
                    ImNodes::AutoPositionNode(actors.back());
                }
            }
            
            ImGui::Separator();
            if (ImGui::MenuItem("Reset Zoom"))
                canvas.zoom = 1;

            if (ImGui::IsAnyMouseDown() && !ImGui::IsWindowHovered())
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        ImNodes::EndCanvas();
    }
    ImGui::End();
    
    return rc;
}

bool Save( const char* configFile ) {
    if ( actors.size() == 0 ) return false;
    
    zconfig_t* config = sphactor_zconfig_new("root");
    for (auto it = actors.begin(); it != actors.end(); it++)
    {
        GActor* actor = *it;
        zconfig_t* actorSection = sphactor_zconfig_append(actor->actor, config);
        
        // Add custom actor data to section
        actor->SerializeActorData(actorSection);

        zconfig_t* connections = zconfig_locate(config, "connections");
        if ( connections == nullptr ) {
            connections = zconfig_new("connections", config);
        }
        
        for (const Connection& connection : actor->connections)
        {
            //only store connections once
            if ( actor != connection.output_node ) continue;
            
            zconfig_t* item = zconfig_new( "con", connections );
            
            GActor *out = (GActor*)connection.output_node;
            GActor *in = (GActor*)connection.input_node;
            
            zconfig_set_value(item,"%s,%s", sphactor_ask_endpoint(out->actor), sphactor_ask_endpoint(in->actor));
        }
    }
    zconfig_save(config, configFile);
    
    return true;
}

bool Load( const char* configFile ) {
    zconfig_t* root = zconfig_load(configFile);
    
    if ( root == nullptr ) return false;
    
    zconfig_t* configActors = zconfig_locate(root, "actors");
    
    // Clear current stage
    //TODO: Maybe ask if people want to save first?
    Clear();
    
    // Get and loop all actors
    zconfig_t* actor = zconfig_locate(configActors, "actor");
    while( actor != nullptr )
    {
        zconfig_t* uuid = zconfig_locate(actor, "uuid");
        zconfig_t* type = zconfig_locate(actor, "type");
        zconfig_t* endpoint = zconfig_locate(actor, "endpoint");
        
        char *uuidStr = zconfig_value(uuid);
        char *typeStr = zconfig_value(type);
        char *endpointStr = zconfig_value(endpoint);
        
        // We're assuming the endpoint is the last thing added by the sphactor actor
        //  from there we ready until we receive null and send that to the high-level actor
        ImVector<char*> *args = new ImVector<char*>();
        zconfig_t *arg = zconfig_next(endpoint);
        while ( arg != nullptr ) {
            args->push_back(zconfig_value(arg));
            arg = zconfig_next(arg);
        }
        
        GActor *gActor = CreateFromType(typeStr, uuidStr);
        gActor->CreateActor();
        
        auto it = args->begin();
        gActor->DeserializeActorData(args, it);
        
        actors.push_back(gActor);
        
        actor = zconfig_next(actor);
        
        free(uuidStr);
        free(typeStr);
        free(endpointStr);
        
        args->clear();
        delete args;
    }
    
    // Get and loop all connections
    zconfig_t* connections = zconfig_locate(root, "connections");
    zconfig_t* con = zconfig_locate( connections, "con");
    while( con != nullptr ) {
        char* conVal = zconfig_value(con);
        
        // Parse comma separated connection string to get the two endpoints
        int i;
        for( i = 0; conVal[i] != '\0'; ++i ) {
            if ( conVal[i] == ',' ) break;
        }
        
        char* output = new char[i+1];
        char* input = new char[strlen(conVal)-i];
        
        char * pch;
        pch = strtok (conVal,",");
        sprintf(output, "%s", pch);
        pch = strtok (NULL, ",");
        sprintf(input, "%s", pch);
        
        // Loop actors and find output actor
        for (auto it = actors.begin(); it != actors.end();)
        {
            GActor* actor = *it;
            
            // We're the output side, so we recreate the connection
            if (streq(sphactor_ask_endpoint(actor->actor), output)) {
                Connection connection;
                GActor* inputActor = Find(input);
                assert(inputActor);
                
                connection.output_node = actor;
                connection.input_node = inputActor;
                //TODO: Fix OSC type assumption -> part of the connection list?
                connection.input_slot = "OSC";
                connection.output_slot = "OSC";
                
                actor->connections.push_back(connection);
                inputActor->connections.push_back(connection);
                
                sphactor_ask_connect( inputActor->actor, sphactor_ask_endpoint(actor->actor) );
                break;
            }
            
            it++;
        }
        
        con = zconfig_next(con);
        delete[] output;
        delete[] input;
    }
    
    free(root);
    
    return true;
}

void Clear() {
    //delete all connections
    for (auto it = actors.begin(); it != actors.end();)
    {
        GActor* actor = *it;
        
        for (auto& connection : actor->connections)
        {
            //delete them once
            if (connection.output_node == actor) {
                ((GActor*) connection.input_node)->DeleteConnection(connection);
            }
        }
        actor->connections.clear();
        it++;
    }
    
    //delete all actors
    for (auto it = actors.begin(); it != actors.end();)
    {
        GActor* actor = *it;
        actor->DestroyActor();
        delete actor;
        it = actors.erase(it);
    }
}

GActor* Find( const char* endpoint ) {
    for (auto it = actors.begin(); it != actors.end();)
    {
        GActor* actor = *it;
        if ( streq( sphactor_ask_endpoint(actor->actor), endpoint)) {
            return actor;
        }
        
        it++;
    }
    
    return nullptr;
}
