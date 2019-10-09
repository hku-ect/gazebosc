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
std::map<std::string, GNode*(*)()> available_nodes {
    //Count Node
    {"Count", []() -> GNode* { return new CountNode(); } },
    //Midi Node
    {"Midi", []() -> GNode* { return new MidiNode(); } },
    //Midi Node
    {"Log", []() -> GNode* { return new LogNode(); } },
    //Client Node
    {"Client", []() -> GNode* { return new ClientNode(); } },
    //Pulse Node
    {"Pulse", []() -> GNode* { return new PulseNode(); } }
};
std::vector<GNode*> nodes;

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
        //counter++;
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
                zconfig_t* item = zconfig_new( "con", connections );
                
                GNode *out = (GNode*)connection.output_node;
                GNode *in = (GNode*)connection.input_node;
                
                zconfig_set_value(item,"%s,%s", sphactor_endpoint(out->actor), sphactor_endpoint(in->actor));
            }
        }
        zconfig_save(config, configFile);
    }
    ImGui::SameLine();
    ImGui::Spacing();
    ImGui::SameLine();
    if (ImGui::Button("Load")) {                           // Buttons return true when clicked (most widgets return true when edited/activated)
        counter++;
    }
    
    ImGui::Spacing();
    ImGui::InputText("Natnet IP", natnetIP, 128);
    if (ImGui::Button("Connect")) {                           // Buttons return true when clicked (most widgets return true when edited/activated)
        counter++;
    }

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::End();
}

namespace ImGui
{
    //TODO: refactor into our own full-screen background editor
    void ShowDemoWindow(bool*)
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
                    for (auto& connection : node->connections)
                    {
                        if (connection.output_node == node) {
                            ((GNode*) connection.input_node)->DeleteConnection(connection);
                            ((GNode*) connection.output_node)->DeleteConnection(connection);
                        }
                        else {
                            ((GNode*) connection.output_node)->DeleteConnection(connection);
                            ((GNode*) connection.input_node)->DeleteConnection(connection);
                        }
                    }
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
                        nodes.push_back(desc.second());
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

    

}   // namespace ImGui
