#ifndef DEMOWINDOW_HPP
#define DEMOWINDOW_HPP

#include "Window.hpp"
#include "imgui.h"

struct DemoWindow : public gzb::Window
{
    DemoWindow() { window_name = "Demo Window"; showing=false; };
    void OnImGui() { ImGui::ShowDemoWindow(&showing); };
};
#endif // DEMOWINDOW_HPP
