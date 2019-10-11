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


//TODO: generate this?
// we probably need to dynamically define inputs and outputs based on nodes written by ourselves/others...
std::map<std::string, GNode*(*)(const char*)> available_nodes {
    //Count Node
    {"Count", [](const char * uuid) -> GNode* { return new CountNode(uuid); } },
    //Midi Node
    {"Midi", [](const char * uuid) -> GNode* { return new MidiNode(uuid); } },
    //Midi Node
    {"Log", [](const char * uuid) -> GNode* { return new LogNode(uuid); } },
    //Client Node
    {"Client", [](const char * uuid) -> GNode* { return new ClientNode(uuid); } },
    //Pulse Node
    {"Pulse", [](const char * uuid) -> GNode* { return new PulseNode(uuid); } }
};
std::vector<GNode*> nodes;

void Save( const char* configFile );
void Load( const char* configFile );
void Clear();
GNode* Find( const char* endpoint );


void ShowConfigWindow() {
    //config window (natnet data, configuration file, connection button
    //TODO: Move to separate cpp file?
    //ImGui::ShowControlWindow(true, )

    static int counter = 0;
    static char* configFile = new char[64];
    static char* natnetIP = new char[64];

    //creates window
    ImGui::Begin("Configuration");

    ImGui::Text("GazebOSC Settings");

    ImGui::InputText("Config-file", configFile, 128);
    
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
    
    ImGui::Spacing();
    ImGui::InputText("Natnet IP", natnetIP, 128);
    if (ImGui::Button("Connect")) {                           // Buttons return true when clicked (most widgets return true when edited/activated)
        counter++;
    }

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::End();
}

//TODO: refactor into our own full-screen background editor
void RenderNodes()
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
                node->RenderUI();

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

            if (node->selected && ImGui::IsKeyPressedMap(ImGuiKey_Backspace))
            {
                // loop and delete connections of nodes connected to us
                for (auto& connection : node->connections)
                {
                    if (connection.output_node == node) {
                        ((GNode*) connection.input_node)->DeleteConnection(connection);
                    }
                    else {
                        ((GNode*) connection.output_node)->DeleteConnection(connection);
                    }
                }
                // delete all our connections separately
                node->connections.clear();
                
                delete node;
                it = nodes.erase(it);
            }
            else
                ++it;
        }

        //const ImGuiIO& io = ImGui::GetIO();
        if (ImGui::IsMouseReleased(1) && ImGui::IsWindowHovered() && !ImGui::IsMouseDragging(1))
        {
            ImGui::FocusWindow(ImGui::GetCurrentWindow());
            ImGui::OpenPopup("NodesContextMenu");
        }

        if (ImGui::BeginPopup("NodesContextMenu"))
        {
            for (const auto& desc : available_nodes)
            {
                if (ImGui::MenuItem(desc.first.c_str()))
                {
                    nodes.push_back(desc.second(nullptr));
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
        node->Serialize(nodeSection);

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

void Load( const char* configFile ) {
    zconfig_t* root = zconfig_load(configFile);
    
    if ( root == nullptr ) return;
    
    zconfig_t* configNodes = zconfig_locate(root, "nodes");
    
    //clear current sketch
    //TODO: Maybe ask if people want to save first?
    Clear();
    
    //recreate nodes
    //TODO: move this to somewhere else?
    
    zconfig_t* node = zconfig_locate(configNodes, "node");
    while( node != nullptr )
    {
        zconfig_t* uuid = zconfig_locate(node, "uuid");
        zconfig_t* type = zconfig_locate(node, "type");
        zconfig_t* endpoint = zconfig_locate(node, "endpoint");
        
        char *uuidStr = zconfig_value(uuid);
        char *typeStr = zconfig_value(type);
        char *endpointStr = zconfig_value(endpoint);
        
        ImVector<char*> *args = new ImVector<char*>();
        zconfig_t *arg = zconfig_next(endpoint);
        while ( arg != nullptr ) {
            args->push_back(zconfig_value(arg));
            arg = zconfig_next(arg);
        }
        
        GNode *gNode = available_nodes[typeStr](uuidStr);
        
        ImVector<char*>::iterator it = args->begin();
        gNode->HandleArgs(args, it);
        
        nodes.push_back(gNode);
        
        node = zconfig_next(node);
        
        free(uuidStr);
        free(typeStr);
        free(endpointStr);
        
        args->clear();
        delete args;
    }
    
    zconfig_t* connections = zconfig_locate(root, "connections");
    zconfig_t* con = zconfig_locate( connections, "con");
    while( con != nullptr ) {
        char* conVal = zconfig_value(con);
        
        //find nodes and connect them
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
        
        //printf("output: %s", output);
        //printf("input: %s", input);
        
        for (auto it = nodes.begin(); it != nodes.end();)
        {
            GNode* node = *it;
            
            //if output is our endpoint
            if (streq(sphactor_endpoint(node->actor), output)) {
                Connection connection;
                GNode* inputNode = Find(input);
                assert(inputNode);
                
                connection.output_node = node;
                connection.input_node = inputNode;
                //TODO: Fix...
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
                ((GNode*) connection.output_node)->DeleteConnection(connection);
            }
        }
        delete node;
        it = nodes.erase(it);
    }
    
    //delete all nodes
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
