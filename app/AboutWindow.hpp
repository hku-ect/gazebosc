#ifndef ABOUTWINDOW_HPP
#define ABOUTWINDOW_HPP

#include "Window.hpp"
#include "czmq.h"

namespace gzb {
class AboutWindow : public Window
{
public:
    AboutWindow();
    ~AboutWindow();
    void OnImGui();

    ziflist_t *nics = NULL; // list of network interfaces
};
} // namespace
#endif // ABOUTWINDOW_HPP
