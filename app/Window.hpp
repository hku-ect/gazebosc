#ifndef GZBWINDOW_HPP
#define GZBWINDOW_HPP

#include <string>

namespace gzb {
struct Window
{
    Window() {};
    virtual ~Window() {};
    virtual void OnImGui() {};

    bool showing = false;
    std::string window_name = "noname";
};
}
#endif // GZBWINDOW_HPP
