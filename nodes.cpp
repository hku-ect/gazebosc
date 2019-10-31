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
#include "GNode.h"
#include "TestNodes.h"
#include "Nodes/DefaultNodes.h"


// List of actor types and constructors
//  Currently these are all internal dependencies, but we will need to create
//   a "generic node" class for types defined outside of this codebase.
ImVector<char*> actor_types;
std::map<std::string, GNode*(*)(const char*)> type_constructors;
std::vector<GNode*> nodes;

void Save( const char* configFile );
bool Load( const char* configFile );
void Clear();
GNode* Find( const char* endpoint );

typedef GNode* (*gnode_constructor_fn)(const char*);

void RegisterNode( const char* type, sphactor_handler_fn func, gnode_constructor_fn constructor ) {
    sphactor_register(type, func);
    type_constructors[type] = constructor;
}

void UpdateRegisteredNodesCache() {
    zlist_t* list = sphactor_get_registered();
    actor_types.clear();
    
    char* item = (char*)zlist_first(list);
    while( item ) {
        actor_types.push_back(item);
        item = (char*)zlist_next(list);
    }
}

GNode* CreateFromType( const char* typeStr, const char* uuidStr ) {
    auto typeIterator = type_constructors.find(typeStr);
    if ( typeIterator != type_constructors.end() ) {
        return type_constructors[typeStr](uuidStr);
    }
    else {
        //TODO: Implement base node class
        zsys_error("Unknown node type encountered: %s -> TODO: implement generic base node", typeStr);
        return new RelayNode(uuidStr);
    }
}

void RegisterCPPNodes() {
    
    // When implementing new nodes, this is the only place you need to add them
    RegisterNode( "Log", GNode::_actor_handler, [](const char * uuid) -> GNode* { return new LogNode(uuid); });
    RegisterNode( "Midi", GNode::_actor_handler, [](const char * uuid) -> GNode* { return new MidiNode(uuid); });
    RegisterNode( "Count", GNode::_actor_handler, [](const char * uuid) -> GNode* { return new CountNode(uuid); });
    RegisterNode( "UDPSend", GNode::_actor_handler, [](const char * uuid) -> GNode* { return new UDPSendNode(uuid); });
    RegisterNode( "Pulse", GNode::_actor_handler, [](const char * uuid) -> GNode* { return new PulseNode(uuid); });
    RegisterNode( "Relay", GNode::_actor_handler, [](const char * uuid) -> GNode* { return new RelayNode(uuid); });
    RegisterNode( "OSCListener", GNode::_actor_handler, [](const char * uuid) -> GNode* { return new OSCListenerNode(uuid); });
    RegisterNode( "ManualPulse", GNode::_actor_handler, [](const char * uuid) -> GNode* { return new ManualPulse(uuid); });
    
    //TODO: Figure out when this needs to happen
    UpdateRegisteredNodesCache();
}

void ShowConfigWindow() {
    static char* configFile = new char[64];

    //creates window
    ImGui::Begin("Sketch Settings", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove );

    ImGui::Text("Save/load sketch");

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

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::End();
}

void UpdateNodes(float deltaTime)
{
    // Canvas must be created after ImGui initializes, because constructor accesses ImGui style to configure default colors.
    static ImNodes::CanvasState canvas{};
    
    //const ImGuiStyle& style = ImGui::GetStyle();
    
    ImGui::SetNextWindowPos(ImVec2(0,0));
    
    if (ImGui::Begin("ImNodes", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
    {
        // We probably need to keep some state, like positions of nodes/slots for rendering connections.
        ImNodes::BeginCanvas(&canvas);
        for (auto it = nodes.begin(); it != nodes.end();)
        {
            GNode* node = *it;

            // Start rendering node
            if (ImNodes::Ez::BeginNode(node, node->title, &node->pos, &node->selected))
            {
                // Render input nodes first (order is important)
                ImNodes::Ez::InputSlots(node->input_slots.data(), node->input_slots.size());

                // Custom node content may go here
                node->Render(deltaTime);

                // Render output nodes first (order is important)
                ImNodes::Ez::OutputSlots(node->output_slots.data(), node->output_slots.size());

                // Store new connections when they are created
                Connection new_connection;
                if (ImNodes::GetNewConnection(&new_connection.input_node, &new_connection.input_slot,
                    &new_connection.output_node, &new_connection.output_slot))
                {
                    assert(new_connection.input_node);
                    assert(new_connection.output_node);
                    ((GNode*) new_connection.input_node)->connections.push_back(new_connection);
                    ((GNode*) new_connection.output_node)->connections.push_back(new_connection);
                    sphactor_connect( ((GNode*) new_connection.input_node)->actor,
                                      sphactor_endpoint( ((GNode*) new_connection.output_node)->actor ) );
                }

                // Render output connections of this node
                for (const Connection& connection : node->connections)
                {
                    // Node contains all it's connections (both from output and to input slots). This means that multiple
                    // nodes will have same connection. We render only output connections and ensure that each connection
                    // will be rendered once.
                    if (connection.output_node != node)
                        continue;

                    if (!ImNodes::Connection(connection.input_node, connection.input_slot, connection.output_node,
                        connection.output_slot))
                    {
                        assert(connection.input_node);
                        assert(connection.output_node);
                        sphactor_disconnect( ((GNode*) connection.input_node)->actor,
                                          sphactor_endpoint( ((GNode*) connection.output_node)->actor ) );
                        // Remove deleted connections
                        ((GNode*) connection.input_node)->DeleteConnection(connection);
                        ((GNode*) connection.output_node)->DeleteConnection(connection);
                    }
                }
            }
            // Node rendering is done. This call will render node background based on size of content inside node.
            ImNodes::Ez::EndNode();
            
            bool del = ImGui::IsKeyPressedMap(ImGuiKey_Delete) || ( ImGui::IsKeyPressedMap(ImGuiKey_Backspace) && ImGui::GetActiveID() == 0 );
            if (node->selected && del)
            {
                // Loop and delete connections of nodes connected to us
                for (auto& connection : node->connections)
                {
                    if (connection.output_node == node) {
                        ((GNode*) connection.input_node)->DeleteConnection(connection);
                    }
                    else {
                        ((GNode*) connection.output_node)->DeleteConnection(connection);
                    }
                }
                // Delete all our connections separately
                node->connections.clear();
                node->DestroyActor();
                
                delete node;
                it = nodes.erase(it);
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
                    GNode* node = CreateFromType(desc, nullptr);
                    node->CreateActor();
                    nodes.push_back(node);
                    ImNodes::AutoPositionNode(nodes.back());
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
}

void Save( const char* configFile ) {
    zconfig_t* config = sphactor_zconfig_new("root");
    for (auto it = nodes.begin(); it != nodes.end(); it++)
    {
        GNode* node = *it;
        zconfig_t* nodeSection = sphactor_zconfig_append(node->actor, config);
        
        // Add custom node data to section
        node->SerializeNodeData(nodeSection);

        zconfig_t* connections = zconfig_locate(config, "connections");
        if ( connections == nullptr ) {
            connections = zconfig_new("connections", config);
        }
        
        for (const Connection& connection : node->connections)
        {
            //only store connections once
            if ( node != connection.output_node ) continue;
            
            zconfig_t* item = zconfig_new( "con", connections );
            
            GNode *out = (GNode*)connection.output_node;
            GNode *in = (GNode*)connection.input_node;
            
            zconfig_set_value(item,"%s,%s", sphactor_endpoint(out->actor), sphactor_endpoint(in->actor));
        }
    }
    zconfig_save(config, configFile);
}

bool Load( const char* configFile ) {
    zconfig_t* root = zconfig_load(configFile);
    
    if ( root == nullptr ) return false;
    
    zconfig_t* configNodes = zconfig_locate(root, "actors");
    
    // Clear current sketch
    //TODO: Maybe ask if people want to save first?
    Clear();
    
    // Get and loop all nodes
    zconfig_t* node = zconfig_locate(configNodes, "actor");
    while( node != nullptr )
    {
        zconfig_t* uuid = zconfig_locate(node, "uuid");
        zconfig_t* type = zconfig_locate(node, "type");
        zconfig_t* endpoint = zconfig_locate(node, "endpoint");
        
        char *uuidStr = zconfig_value(uuid);
        char *typeStr = zconfig_value(type);
        char *endpointStr = zconfig_value(endpoint);
        
        // We're assuming the endpoint is the last thing added by the sphactor actor
        //  from there we ready until we receive null and send that to the high-level node
        ImVector<char*> *args = new ImVector<char*>();
        zconfig_t *arg = zconfig_next(endpoint);
        while ( arg != nullptr ) {
            args->push_back(zconfig_value(arg));
            arg = zconfig_next(arg);
        }
        
        GNode *gNode = CreateFromType(typeStr, uuidStr);
        gNode->CreateActor();
        
        auto it = args->begin();
        gNode->DeserializeNodeData(args, it);
        
        nodes.push_back(gNode);
        
        node = zconfig_next(node);
        
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
        
        // Loop nodes and find output actor
        for (auto it = nodes.begin(); it != nodes.end();)
        {
            GNode* node = *it;
            
            // We're the output node, so we recreate the connection
            if (streq(sphactor_endpoint(node->actor), output)) {
                Connection connection;
                GNode* inputNode = Find(input);
                assert(inputNode);
                
                connection.output_node = node;
                connection.input_node = inputNode;
                //TODO: Fix OSC type assumption -> part of the connection list?
                connection.input_slot = "OSC";
                connection.output_slot = "OSC";
                
                node->connections.push_back(connection);
                inputNode->connections.push_back(connection);
                
                sphactor_connect( inputNode->actor, sphactor_endpoint(node->actor) );
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
    for (auto it = nodes.begin(); it != nodes.end();)
    {
        GNode* node = *it;
        
        for (auto& connection : node->connections)
        {
            //delete them once
            if (connection.output_node == node) {
                ((GNode*) connection.input_node)->DeleteConnection(connection);
            }
        }
        node->connections.clear();
        it++;
    }
    
    //delete all nodes
    for (auto it = nodes.begin(); it != nodes.end();)
    {
        GNode* node = *it;
        node->DestroyActor();
        delete node;
        it = nodes.erase(it);
    }
}

GNode* Find( const char* endpoint ) {
    for (auto it = nodes.begin(); it != nodes.end();)
    {
        GNode* node = *it;
        if ( streq( sphactor_endpoint(node->actor), endpoint)) {
            return node;
        }
        
        it++;
    }
    
    return nullptr;
}
