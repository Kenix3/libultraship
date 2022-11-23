#include "InputEditor.h"
#include "controller/Controller.h"
#include "core/Window.h"
#include <ImGui/imgui.h>
#include "ImGuiImpl.h"
#include <Utils/StringHelper.h>
#include <ImGui/imgui_internal.h>
#include "core/bridge/consolevariablebridge.h"

namespace Ship {

#define SEPARATION() ImGui::Dummy(ImVec2(0, 5))

void InputEditor::Init() {
    mBtnReading = -1;
}

std::shared_ptr<Controller> GetControllerPerSlot(int slot) {
    auto controlDeck = Ship::Window::GetInstance()->GetControlDeck();
    return controlDeck->GetPhysicalDeviceFromVirtualSlot(slot);
}

void InputEditor::DrawButton(const char* label, int32_t n64Btn, int32_t currentPort, int32_t* btnReading) {
    const std::shared_ptr<Controller> backend = GetControllerPerSlot(currentPort);

    float size = 40;
    bool readingMode = *btnReading == n64Btn;
    bool disabled = (*btnReading != -1 && !readingMode) || !backend->Connected() || backend->GetGuid() == "Auto";
    ImVec2 len = ImGui::CalcTextSize(label);
    ImVec2 pos = ImGui::GetCursorPos();
    ImGui::SetCursorPosY(pos.y + len.y / 4);
    ImGui::SetCursorPosX(pos.x + abs(len.x - size));
    ImGui::Text("%s", label);
    ImGui::SameLine();
    ImGui::SetCursorPosY(pos.y);

    if (disabled) {
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
    }

    if (readingMode) {
        const int32_t btn = backend->ReadRawPress();

        if (btn != -1) {
            backend->SetButtonMapping(currentPort, n64Btn, btn);
            *btnReading = -1;

            // avoid immediately triggering another button during gamepad nav
            ImGui::SetKeyboardFocusHere(0);
        }
    }

    const std::string btnName = backend->GetButtonName(currentPort, n64Btn);

    if (ImGui::Button(
            StringHelper::Sprintf("%s##HBTNID_%d", readingMode ? "Press a Key..." : btnName.c_str(), n64Btn).c_str())) {
        *btnReading = n64Btn;
        backend->ClearRawPress();
    }

    if (disabled) {
        ImGui::PopItemFlag();
        ImGui::PopStyleVar();
    }
}

void InputEditor::DrawControllerSelect(int32_t currentPort) {
    auto controlDeck = Ship::Window::GetInstance()->GetControlDeck();
    std::string controllerName = controlDeck->GetPhysicalDeviceFromVirtualSlot(currentPort)->GetControllerName();

    if (ImGui::BeginCombo("##ControllerEntries", controllerName.c_str())) {
        for (uint8_t i = 0; i < controlDeck->GetNumPhysicalDevices(); i++) {
            std::string deviceName = controlDeck->GetPhysicalDevice(i)->GetControllerName();
            if (deviceName != "Keyboard" && deviceName != "Auto") {
                deviceName += "##" + std::to_string(i);
            }
            if (ImGui::Selectable(deviceName.c_str(), i == controlDeck->GetVirtualDevice(currentPort))) {
                controlDeck->SetPhysicalDevice(currentPort, i);
            }
        }
        ImGui::EndCombo();
    }

    ImGui::SameLine();

    if (ImGui::Button("Refresh")) {
        controlDeck->ScanPhysicalDevices();
    }
}

void InputEditor::DrawVirtualStick(const char* label, ImVec2 stick) {
    ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPos().x + 5, ImGui::GetCursorPos().y));
    ImGui::BeginChild(label, ImVec2(68, 75), false);
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const ImVec2 p = ImGui::GetCursorScreenPos();

    float sz = 45.0f;
    float rad = sz * 0.5f;
    ImVec2 pos = ImVec2(p.x + sz * 0.5f + 12, p.y + sz * 0.5f + 11);

    float stickX = (stick.x / 83.0f) * (rad * 0.5f);
    float stickY = -(stick.y / 83.0f) * (rad * 0.5f);

    ImVec4 rect = ImVec4(p.x + 2, p.y + 2, 65, 65);
    drawList->AddRect(ImVec2(rect.x, rect.y), ImVec2(rect.x + rect.z, rect.y + rect.w), ImColor(100, 100, 100, 255),
                      0.0f, 0, 1.5f);
    drawList->AddCircleFilled(pos, rad, ImColor(130, 130, 130, 255), 8);
    drawList->AddCircleFilled(ImVec2(pos.x + stickX, pos.y + stickY), 5, ImColor(15, 15, 15, 255), 7);
    ImGui::EndChild();
}

void InputEditor::DrawControllerSchema() {
    auto backend = Ship::Window::GetInstance()->GetControlDeck()->GetPhysicalDeviceFromVirtualSlot(mCurrentPort);
    auto profile = backend->getProfile(mCurrentPort);
    bool isKeyboard = backend->GetGuid() == "Keyboard" || backend->GetGuid() == "Auto" || !backend->Connected();

    DrawControllerSelect(mCurrentPort);

    SohImGui::BeginGroupPanel("Buttons", ImVec2(150, 20));
    DrawButton("A", BTN_A, mCurrentPort, &mBtnReading);
    DrawButton("B", BTN_B, mCurrentPort, &mBtnReading);
    DrawButton("L", BTN_L, mCurrentPort, &mBtnReading);
    DrawButton("R", BTN_R, mCurrentPort, &mBtnReading);
    DrawButton("Z", BTN_Z, mCurrentPort, &mBtnReading);
    DrawButton("START", BTN_START, mCurrentPort, &mBtnReading);
    SEPARATION();
#ifdef __SWITCH__
    SohImGui::EndGroupPanel(isKeyboard ? 7.0f : 56.0f);
#else
    SohImGui::EndGroupPanel(isKeyboard ? 7.0f : 48.0f);
#endif
    ImGui::SameLine();
    SohImGui::BeginGroupPanel("Digital Pad", ImVec2(150, 20));
    DrawButton("Up", BTN_DUP, mCurrentPort, &mBtnReading);
    DrawButton("Down", BTN_DDOWN, mCurrentPort, &mBtnReading);
    DrawButton("Left", BTN_DLEFT, mCurrentPort, &mBtnReading);
    DrawButton("Right", BTN_DRIGHT, mCurrentPort, &mBtnReading);
    SEPARATION();
#ifdef __SWITCH__
    SohImGui::EndGroupPanel(isKeyboard ? 53.0f : 122.0f);
#else
    SohImGui::EndGroupPanel(isKeyboard ? 53.0f : 94.0f);
#endif
    ImGui::SameLine();
    SohImGui::BeginGroupPanel("Analog Stick", ImVec2(150, 20));
    DrawButton("Up", BTN_STICKUP, mCurrentPort, &mBtnReading);
    DrawButton("Down", BTN_STICKDOWN, mCurrentPort, &mBtnReading);
    DrawButton("Left", BTN_STICKLEFT, mCurrentPort, &mBtnReading);
    DrawButton("Right", BTN_STICKRIGHT, mCurrentPort, &mBtnReading);

    if (!isKeyboard) {
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8);
        DrawVirtualStick("##MainVirtualStick",
                         ImVec2(backend->getLeftStickX(mCurrentPort), backend->getLeftStickY(mCurrentPort)));
        ImGui::SameLine();

        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 5);

#ifdef __WIIU__
        ImGui::BeginChild("##MSInput", ImVec2(90 * 2, 50 * 2), false);
#else
        ImGui::BeginChild("##MSInput", ImVec2(90, 50), false);
#endif
        ImGui::Text("Deadzone");
#ifdef __WIIU__
        ImGui::PushItemWidth(80 * 2);
#else
        ImGui::PushItemWidth(80);
#endif
        // The window has deadzone per stick, so we need to
        // set the deadzone for both left stick axes here
        // SDL_CONTROLLER_AXIS_LEFTX: 0
        // SDL_CONTROLLER_AXIS_LEFTY: 1
        ImGui::InputFloat("##MDZone", &profile->AxisDeadzones[0], 1.0f, 0.0f, "%.0f");
        profile->AxisDeadzones[1] = profile->AxisDeadzones[0];
        ImGui::PopItemWidth();
        ImGui::EndChild();
    } else {
        ImGui::Dummy(ImVec2(0, 6));
    }
#ifdef __SWITCH__
    SohImGui::EndGroupPanel(isKeyboard ? 52.0f : 52.0f);
#else
    SohImGui::EndGroupPanel(isKeyboard ? 52.0f : 24.0f);
#endif
    ImGui::SameLine();

    if (!isKeyboard) {
        ImGui::SameLine();
        SohImGui::BeginGroupPanel("Right Stick", ImVec2(150, 20));
        DrawButton("Up", BTN_VSTICKUP, mCurrentPort, &mBtnReading);
        DrawButton("Down", BTN_VSTICKDOWN, mCurrentPort, &mBtnReading);
        DrawButton("Left", BTN_VSTICKLEFT, mCurrentPort, &mBtnReading);
        DrawButton("Right", BTN_VSTICKRIGHT, mCurrentPort, &mBtnReading);

        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8);
        // 2 is the SDL value for right stick X axis
        // 3 is the SDL value for right stick Y axis.
        DrawVirtualStick("##RightVirtualStick",
                         ImVec2(backend->getRightStickX(mCurrentPort), backend->getRightStickY(mCurrentPort)));

        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 5);
#ifdef __WIIU__
        ImGui::BeginChild("##CSInput", ImVec2(90 * 2, 85 * 2), false);
#else
        ImGui::BeginChild("##CSInput", ImVec2(90, 85), false);
#endif
        ImGui::Text("Deadzone");
#ifdef __WIIU__
        ImGui::PushItemWidth(80 * 2);
#else
        ImGui::PushItemWidth(80);
#endif
        // The window has deadzone per stick, so we need to
        // set the deadzone for both right stick axes here
        // SDL_CONTROLLER_AXIS_RIGHTX: 2
        // SDL_CONTROLLER_AXIS_RIGHTY: 3
        ImGui::InputFloat("##MDZone", &profile->AxisDeadzones[2], 1.0f, 0.0f, "%.0f");
        profile->AxisDeadzones[3] = profile->AxisDeadzones[2];
        ImGui::PopItemWidth();
        ImGui::EndChild();
#ifdef __SWITCH__
        SohImGui::EndGroupPanel(43.0f);
#else
        SohImGui::EndGroupPanel(14.0f);
#endif
    }

    if (backend->CanGyro()) {
#ifndef __WIIU__
        ImGui::SameLine();
#endif
        SohImGui::BeginGroupPanel("Gyro Options", ImVec2(175, 20));
        float cursorX = ImGui::GetCursorPosX() + 5;
        ImGui::SetCursorPosX(cursorX);
        ImGui::Checkbox("Enable Gyro", &profile->UseGyro);
        ImGui::SetCursorPosX(cursorX);
        ImGui::Text("Gyro Sensitivity: %d%%", static_cast<int>(100.0f * profile->GyroData[GYRO_SENSITIVITY]));
#ifdef __WIIU__
        ImGui::PushItemWidth(135.0f * 2);
#else
        ImGui::PushItemWidth(135.0f);
#endif
        ImGui::SetCursorPosX(cursorX);
        ImGui::SliderFloat("##GSensitivity", &profile->GyroData[GYRO_SENSITIVITY], 0.0f, 1.0f, "");
        ImGui::PopItemWidth();
        ImGui::Dummy(ImVec2(0, 1));
        ImGui::SetCursorPosX(cursorX);
        if (ImGui::Button("Recalibrate Gyro##RGyro")) {
            profile->GyroData[DRIFT_X] = 0.0f;
            profile->GyroData[DRIFT_Y] = 0.0f;
        }
        ImGui::SetCursorPosX(cursorX);
        DrawVirtualStick("##GyroPreview",
                         ImVec2(-10.0f * backend->getGyroY(mCurrentPort), 10.0f * backend->getGyroX(mCurrentPort)));

        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 5);
#ifdef __WIIU__
        ImGui::BeginChild("##GyInput", ImVec2(90 * 2, 85 * 2), false);
#else
        ImGui::BeginChild("##GyInput", ImVec2(90, 85), false);
#endif
        ImGui::Text("Drift X");
#ifdef __WIIU__
        ImGui::PushItemWidth(80 * 2);
#else
        ImGui::PushItemWidth(80);
#endif
        ImGui::InputFloat("##GDriftX", &profile->GyroData[DRIFT_X], 1.0f, 0.0f, "%.1f");
        ImGui::PopItemWidth();
        ImGui::Text("Drift Y");
#ifdef __WIIU__
        ImGui::PushItemWidth(80 * 2);
#else
        ImGui::PushItemWidth(80);
#endif
        ImGui::InputFloat("##GDriftY", &profile->GyroData[DRIFT_Y], 1.0f, 0.0f, "%.1f");
        ImGui::PopItemWidth();
        ImGui::EndChild();
#ifdef __SWITCH__
        SohImGui::EndGroupPanel(46.0f);
#else
        SohImGui::EndGroupPanel(14.0f);
#endif
    }

    ImGui::SameLine();

    const ImVec2 cursor = ImGui::GetCursorPos();

    SohImGui::BeginGroupPanel("C-Buttons", ImVec2(158, 20));
    DrawButton("Up", BTN_CUP, mCurrentPort, &mBtnReading);
    DrawButton("Down", BTN_CDOWN, mCurrentPort, &mBtnReading);
    DrawButton("Left", BTN_CLEFT, mCurrentPort, &mBtnReading);
    DrawButton("Right", BTN_CRIGHT, mCurrentPort, &mBtnReading);
    ImGui::Dummy(ImVec2(0, 5));
    SohImGui::EndGroupPanel();

    ImGui::SetCursorPosX(cursor.x);
#ifdef __SWITCH__
    ImGui::SetCursorPosY(cursor.y + 167);
#elif defined(__WIIU__)
    ImGui::SetCursorPosY(cursor.y + 120 * 2);
#else
    ImGui::SetCursorPosY(cursor.y + 120);
#endif
    SohImGui::BeginGroupPanel("Options", ImVec2(158, 20));
    float cursorX = ImGui::GetCursorPosX() + 5;
    ImGui::SetCursorPosX(cursorX);
    ImGui::Checkbox("Rumble Enabled", &profile->UseRumble);
    if (backend->CanRumble()) {
        ImGui::SetCursorPosX(cursorX);
        ImGui::Text("Rumble Force: %d%%", static_cast<int>(100.0f * profile->RumbleStrength));
        ImGui::SetCursorPosX(cursorX);
#ifdef __WIIU__
        ImGui::PushItemWidth(135.0f * 2);
#else
        ImGui::PushItemWidth(135.0f);
#endif
        ImGui::SliderFloat("##RStrength", &profile->RumbleStrength, 0.0f, 1.0f, "");
        ImGui::PopItemWidth();
    }
    ImGui::Dummy(ImVec2(0, 5));
    SohImGui::EndGroupPanel(isKeyboard ? 0.0f : 2.0f);
}

void InputEditor::DrawHud() {
    if (!this->mOpened) {
        mBtnReading = -1;
        CVarSetInteger("gControllerConfigurationEnabled", 0);
        return;
    }

#ifdef __SWITCH__
    ImVec2 minSize = ImVec2(641, 250);
    ImVec2 maxSize = ImVec2(2200, 505);
#elif defined(__WIIU__)
    ImVec2 minSize = ImVec2(641 * 2, 250 * 2);
    ImVec2 maxSize = ImVec2(1200 * 2, 290 * 2);
#else
    ImVec2 minSize = ImVec2(641, 250);
    ImVec2 maxSize = ImVec2(1200, 290);
#endif

    ImGui::SetNextWindowSizeConstraints(minSize, maxSize);
    // OTRTODO: Disable this stupid workaround ( ReadRawPress() only works when the window is on the main viewport )
    ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
    ImGui::Begin("Controller Configuration", &this->mOpened,
                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::BeginTabBar("##Controllers");

    for (int i = 0; i < 4; i++) {
        if (ImGui::BeginTabItem(StringHelper::Sprintf("Port %d", i + 1).c_str())) {
            mCurrentPort = i;
            ImGui::EndTabItem();
        }
    }

    ImGui::EndTabBar();

    // Draw current cfg

    DrawControllerSchema();

    ImGui::End();
}

bool InputEditor::IsOpened() {
    return mOpened;
}

void InputEditor::Open() {
    mOpened = true;
}

void InputEditor::Close() {
    mOpened = false;
}
} // namespace Ship
