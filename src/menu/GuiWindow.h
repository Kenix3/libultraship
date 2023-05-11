#pragma once

#include <string>

namespace LUS {
class GuiWindow {
public:
    GuiWindow(const std::string& name, bool isOpen);
    GuiWindow(const std::string& name);

    virtual void Init() = 0;
    virtual void Draw() = 0;
    virtual void Update() = 0;

    std::string GetName();
    void Open();
    void Close();
    bool IsOpen();
protected:
    bool mIsOpen;
protected:
    std::string mName;
};
} // namespace LUS
