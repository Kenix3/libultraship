#include "InputEditorWindow.h"
// #include "controller/KeyboardController.h"
#include "Context.h"
#include "Gui.h"
#include <Utils/StringHelper.h>
#include "public/bridge/consolevariablebridge.h"

namespace LUS {

InputEditorWindow::~InputEditorWindow() {
    SPDLOG_TRACE("destruct input editor window");
}

void InputEditorWindow::InitElement() {
    mBtnReading = -1;
    mGameInputBlockTimer = INT32_MAX;
}

// std::shared_ptr<Controller> GetControllerPerSlot(int slot) {
//     auto controlDeck = Context::GetInstance()->GetControlDeck();
//     return controlDeck->GetDeviceFromPortIndex(slot);
// }

// void InputEditorWindow::DrawButton(const char* label, int32_t n64Btn, int32_t currentPort, int32_t* btnReading) {
//     const std::shared_ptr<Controller> backend = GetControllerPerSlot(currentPort);

//     float size = 40;
//     bool readingMode = *btnReading == n64Btn;
//     bool disabled = (*btnReading != -1 && !readingMode) || !backend->Connected() || backend->GetGuid() == "Auto";
//     ImVec2 len = ImGui::CalcTextSize(label);
//     ImVec2 pos = ImGui::GetCursorPos();
//     ImGui::SetCursorPosY(pos.y + len.y / 4);
//     ImGui::SetCursorPosX(pos.x + abs(len.x - size));
//     ImGui::Text("%s", label);
//     ImGui::SameLine();
//     ImGui::SetCursorPosY(pos.y);

//     if (disabled) {
//         ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
//         ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
//     }

//     if (readingMode) {
//         const int32_t btn = backend->ReadRawPress();
//         Context::GetInstance()->GetControlDeck()->BlockGameInput(mGameInputBlockId);

//         if (btn != -1) {
//             auto profile = backend->GetProfile(currentPort);
//             // Remove other mappings that include the n64 bitmask. Note that the n64 button is really a mask and is
//             not
//             // unique, but the UI as-is needs a way to unset the old n64 button.
//             std::erase_if(profile->Mappings,
//                           [n64Btn](const std::pair<int32_t, int32_t>& bin) { return bin.second == n64Btn; });
//             backend->SetButtonMapping(currentPort, btn, n64Btn);
//             auto keyboardBackend = dynamic_pointer_cast<KeyboardController>(backend);
//             if (keyboardBackend != nullptr) {
//                 // Window backend has not called release on the scancode and thus we need to release all buttons.
//                 keyboardBackend->ReleaseAllButtons();
//             }
//             Context::GetInstance()->GetControlDeck()->SaveSettings();

//             *btnReading = -1;

//             // avoid immediately triggering another button during gamepad nav
//             ImGui::SetKeyboardFocusHere(0);

//             // don't send input to the game for a third of a second after getting the mapping
//             mGameInputBlockTimer = ImGui::GetIO().Framerate / 3;
//         }
//     }

//     const std::string btnName = backend->GetButtonName(currentPort, n64Btn);

//     if (ImGui::Button(
//             StringHelper::Sprintf("%s##HBTNID_%d", readingMode ? "Press a Key..." : btnName.c_str(),
//             n64Btn).c_str())) {
//         *btnReading = n64Btn;
//         backend->ClearRawPress();
//     }

//     if (disabled) {
//         ImGui::PopItemFlag();
//         ImGui::PopStyleVar();
//     }
// }

// void InputEditorWindow::DrawControllerSelect(int32_t currentPort) {
//     auto controlDeck = Context::GetInstance()->GetControlDeck();
//     std::string controllerName = controlDeck->GetDeviceFromPortIndex(currentPort)->GetControllerName();

//     if (ImGui::BeginCombo("##ControllerEntries", controllerName.c_str())) {
//         for (uint8_t i = 0; i < controlDeck->GetNumDevices(); i++) {
//             std::string deviceName = controlDeck->GetDeviceFromDeviceIndex(i)->GetControllerName();
//             if (deviceName != "Keyboard" && deviceName != "Auto") {
//                 deviceName += "##" + std::to_string(i);
//             }
//             if (ImGui::Selectable(deviceName.c_str(), i == controlDeck->GetDeviceIndexFromPortIndex(currentPort))) {
//                 controlDeck->SetDeviceToPort(currentPort, i);
//                 Context::GetInstance()->GetControlDeck()->SaveSettings();
//             }
//         }
//         ImGui::EndCombo();
//     }

//     ImGui::SameLine();

//     if (ImGui::Button("Refresh")) {
//         controlDeck->ScanDevices();
//     }
// }

// void InputEditorWindow::DrawControllerSchema() {
//     auto backend = Context::GetInstance()->GetControlDeck()->GetDeviceFromPortIndex(mCurrentPort);
//     auto profile = backend->GetProfile(mCurrentPort);
//     bool isKeyboard = backend->GetGuid() == "Keyboard" || backend->GetGuid() == "Auto" || !backend->Connected();

//     backend->ReadToPad(nullptr, mCurrentPort);

//     DrawControllerSelect(mCurrentPort);

//     BeginGroupPanel("Buttons", ImVec2(150, 20));
//     DrawButton("A", BTN_A, mCurrentPort, &mBtnReading);
//     DrawButton("B", BTN_B, mCurrentPort, &mBtnReading);
//     DrawButton("L", BTN_L, mCurrentPort, &mBtnReading);
//     DrawButton("R", BTN_R, mCurrentPort, &mBtnReading);
//     DrawButton("Z", BTN_Z, mCurrentPort, &mBtnReading);
//     DrawButton("START", BTN_START, mCurrentPort, &mBtnReading);
//     SEPARATION();
// #ifdef __SWITCH__
//     EndGroupPanel(isKeyboard ? 7.0f : 56.0f);
// #else
//     EndGroupPanel(isKeyboard ? 7.0f : 48.0f);
// #endif
//     ImGui::SameLine();
//     BeginGroupPanel("Digital Pad", ImVec2(150, 20));
//     DrawButton("Up", BTN_DUP, mCurrentPort, &mBtnReading);
//     DrawButton("Down", BTN_DDOWN, mCurrentPort, &mBtnReading);
//     DrawButton("Left", BTN_DLEFT, mCurrentPort, &mBtnReading);
//     DrawButton("Right", BTN_DRIGHT, mCurrentPort, &mBtnReading);
//     SEPARATION();
// #ifdef __SWITCH__
//     EndGroupPanel(isKeyboard ? 53.0f : 122.0f);
// #else
//     EndGroupPanel(isKeyboard ? 53.0f : 94.0f);
// #endif
//     ImGui::SameLine();
//     BeginGroupPanel("Analog Stick", ImVec2(150, 20));
//     DrawButton("Up", BTN_STICKUP, mCurrentPort, &mBtnReading);
//     DrawButton("Down", BTN_STICKDOWN, mCurrentPort, &mBtnReading);
//     DrawButton("Left", BTN_STICKLEFT, mCurrentPort, &mBtnReading);
//     DrawButton("Right", BTN_STICKRIGHT, mCurrentPort, &mBtnReading);

//     if (!isKeyboard) {
//         ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8);
//         // DrawVirtualStick("##MainVirtualStick",
//         //                  ImVec2(backend->GetLeftStickX(mCurrentPort), backend->GetLeftStickY(mCurrentPort)),
//         //                  profile->AxisDeadzones[0]);
//         ImGui::SameLine();

//         ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 5);

// #ifdef __WIIU__
//         ImGui::BeginChild("##MSInput", ImVec2(90 * 2, 50 * 2), false);
// #else
//         ImGui::BeginChild("##MSInput", ImVec2(90, 50), false);
// #endif
//         ImGui::Text("Deadzone");
// #ifdef __WIIU__
//         ImGui::PushItemWidth(80 * 2);
// #else
//         ImGui::PushItemWidth(80);
// #endif
//         // The window has deadzone per stick, so we need to
//         // set the deadzone for both left stick axes here
//         // SDL_CONTROLLER_AXIS_LEFTX: 0
//         // SDL_CONTROLLER_AXIS_LEFTY: 1
//         if (ImGui::InputFloat("##MDZone", &profile->AxisDeadzones[0], 1.0f, 0.0f, "%.0f")) {
//             if (profile->AxisDeadzones[0] < 0) {
//                 profile->AxisDeadzones[0] = 0;
//             }
//             profile->AxisDeadzones[1] = profile->AxisDeadzones[0];
//             Context::GetInstance()->GetControlDeck()->SaveSettings();
//         }
//         ImGui::PopItemWidth();
//         ImGui::EndChild();
//     } else {
//         ImGui::Dummy(ImVec2(0, 6));
//     }
// #ifdef __SWITCH__
//     EndGroupPanel(isKeyboard ? 52.0f : 52.0f);
// #else
//     EndGroupPanel(isKeyboard ? 52.0f : 24.0f);
// #endif
//     ImGui::SameLine();

//     if (!isKeyboard) {
//         ImGui::SameLine();
//         BeginGroupPanel("Right Stick", ImVec2(150, 20));
//         DrawButton("Up", BTN_VSTICKUP, mCurrentPort, &mBtnReading);
//         DrawButton("Down", BTN_VSTICKDOWN, mCurrentPort, &mBtnReading);
//         DrawButton("Left", BTN_VSTICKLEFT, mCurrentPort, &mBtnReading);
//         DrawButton("Right", BTN_VSTICKRIGHT, mCurrentPort, &mBtnReading);

//         ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8);
//         // 2 is the SDL value for right stick X axis
//         // 3 is the SDL value for right stick Y axis.
//         // DrawVirtualStick("##RightVirtualStick",
//         //                  ImVec2(backend->GetRightStickX(mCurrentPort), backend->GetRightStickY(mCurrentPort)),
//         //                  profile->AxisDeadzones[2]);

//         ImGui::SameLine();
//         ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 5);
// #ifdef __WIIU__
//         ImGui::BeginChild("##CSInput", ImVec2(90 * 2, 85 * 2), false);
// #else
//         ImGui::BeginChild("##CSInput", ImVec2(90, 85), false);
// #endif
//         ImGui::Text("Deadzone");
// #ifdef __WIIU__
//         ImGui::PushItemWidth(80 * 2);
// #else
//         ImGui::PushItemWidth(80);
// #endif
//         // The window has deadzone per stick, so we need to
//         // set the deadzone for both right stick axes here
//         // SDL_CONTROLLER_AXIS_RIGHTX: 2
//         // SDL_CONTROLLER_AXIS_RIGHTY: 3
//         if (ImGui::InputFloat("##MDZone", &profile->AxisDeadzones[2], 1.0f, 0.0f, "%.0f")) {
//             if (profile->AxisDeadzones[2] < 0) {
//                 profile->AxisDeadzones[2] = 0;
//             }
//             Context::GetInstance()->GetControlDeck()->SaveSettings();
//             profile->AxisDeadzones[3] = profile->AxisDeadzones[2];
//         }
//         ImGui::PopItemWidth();
//         ImGui::EndChild();
// #ifdef __SWITCH__
//         EndGroupPanel(43.0f);
// #else
//         EndGroupPanel(14.0f);
// #endif
//     }

//     if (backend->CanGyro()) {
// #ifndef __WIIU__
//         ImGui::SameLine();
// #endif
//         BeginGroupPanel("Gyro Options", ImVec2(175, 20));
//         float cursorX = ImGui::GetCursorPosX() + 5;
//         ImGui::SetCursorPosX(cursorX);
//         if (ImGui::Checkbox("Enable Gyro", &profile->UseGyro)) {
//             Context::GetInstance()->GetControlDeck()->SaveSettings();
//         }
//         ImGui::SetCursorPosX(cursorX);
//         ImGui::Text("Gyro Sensitivity: %d%%", static_cast<int>(100.0f * profile->GyroData[GYRO_SENSITIVITY]));
// #ifdef __WIIU__
//         ImGui::PushItemWidth(135.0f * 2);
// #else
//         ImGui::PushItemWidth(135.0f);
// #endif
//         ImGui::SetCursorPosX(cursorX);
//         if (ImGui::SliderFloat("##GSensitivity", &profile->GyroData[GYRO_SENSITIVITY], 0.0f, 1.0f, "")) {
//             Context::GetInstance()->GetControlDeck()->SaveSettings();
//         }
//         ImGui::PopItemWidth();
//         ImGui::Dummy(ImVec2(0, 1));
//         ImGui::SetCursorPosX(cursorX);
//         if (ImGui::Button("Recalibrate Gyro##RGyro")) {
//             profile->GyroData[DRIFT_X] = 0.0f;
//             profile->GyroData[DRIFT_Y] = 0.0f;
//             Context::GetInstance()->GetControlDeck()->SaveSettings();
//         }
//         ImGui::SetCursorPosX(cursorX);
//         // DrawVirtualStick("##GyroPreview",
//         //                  ImVec2(-10.0f * backend->GetGyroY(mCurrentPort), 10.0f *
//         backend->GetGyroX(mCurrentPort)));

//         ImGui::SameLine();
//         ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 5);
// #ifdef __WIIU__
//         ImGui::BeginChild("##GyInput", ImVec2(90 * 2, 85 * 2), false);
// #else
//         ImGui::BeginChild("##GyInput", ImVec2(90, 85), false);
// #endif
//         ImGui::Text("Drift X");
// #ifdef __WIIU__
//         ImGui::PushItemWidth(80 * 2);
// #else
//         ImGui::PushItemWidth(80);
// #endif
//         if (ImGui::InputFloat("##GDriftX", &profile->GyroData[DRIFT_X], 1.0f, 0.0f, "%.1f")) {
//             Context::GetInstance()->GetControlDeck()->SaveSettings();
//         }
//         ImGui::PopItemWidth();
//         ImGui::Text("Drift Y");
// #ifdef __WIIU__
//         ImGui::PushItemWidth(80 * 2);
// #else
//         ImGui::PushItemWidth(80);
// #endif
//         if (ImGui::InputFloat("##GDriftY", &profile->GyroData[DRIFT_Y], 1.0f, 0.0f, "%.1f")) {
//             Context::GetInstance()->GetControlDeck()->SaveSettings();
//         }
//         ImGui::PopItemWidth();
//         ImGui::EndChild();
// #ifdef __SWITCH__
//         EndGroupPanel(46.0f);
// #else
//         EndGroupPanel(14.0f);
// #endif
//     }

//     ImGui::SameLine();

//     const ImVec2 cursor = ImGui::GetCursorPos();

//     BeginGroupPanel("C-Buttons", ImVec2(158, 20));
//     DrawButton("Up", BTN_CUP, mCurrentPort, &mBtnReading);
//     DrawButton("Down", BTN_CDOWN, mCurrentPort, &mBtnReading);
//     DrawButton("Left", BTN_CLEFT, mCurrentPort, &mBtnReading);
//     DrawButton("Right", BTN_CRIGHT, mCurrentPort, &mBtnReading);
//     ImGui::Dummy(ImVec2(0, 5));
// #ifdef __SWITCH__
//     EndGroupPanel(isKeyboard ? 53.0f : 122.0f);
// #else
//     EndGroupPanel(isKeyboard ? 53.0f : 94.0f);
// #endif

//     ImGui::SetCursorPosX(cursor.x);
//     ImGui::SameLine();

//     BeginGroupPanel("Options", ImVec2(158, 20));
//     float cursorX = ImGui::GetCursorPosX() + 5;
//     ImGui::SetCursorPosX(cursorX);
//     if (ImGui::Checkbox("Rumble Enabled", &profile->UseRumble)) {
//         Context::GetInstance()->GetControlDeck()->SaveSettings();
//     }
//     if (backend->CanRumble()) {
//         ImGui::SetCursorPosX(cursorX);
//         ImGui::Text("Rumble Force: %d%%", static_cast<int32_t>(100.0f * profile->RumbleStrength));
//         ImGui::SetCursorPosX(cursorX);
// #ifdef __WIIU__
//         ImGui::PushItemWidth(135.0f * 2);
// #else
//         ImGui::PushItemWidth(135.0f);
// #endif
//         if (ImGui::SliderFloat("##RStrength", &profile->RumbleStrength, 0.0f, 1.0f, "")) {
//             Context::GetInstance()->GetControlDeck()->SaveSettings();
//         }
//         ImGui::PopItemWidth();
//     }

//     if (!isKeyboard) {
//         ImGui::SetCursorPosX(cursorX);
//         ImGui::Text("Notch Snap Angle: %d", profile->NotchProximityThreshold);
//         ImGui::SetCursorPosX(cursorX);

// #ifdef __WIIU__
//         ImGui::PushItemWidth(135.0f * 2);
// #else
//         ImGui::PushItemWidth(135.0f);
// #endif
//         if (ImGui::SliderInt("##NotchProximityThreshold", &profile->NotchProximityThreshold, 0, 45, "",
//                              ImGuiSliderFlags_AlwaysClamp)) {
//             Context::GetInstance()->GetControlDeck()->SaveSettings();
//         }
//         ImGui::PopItemWidth();
//         if (ImGui::IsItemHovered()) {
//             ImGui::SetTooltip(
//                 "%s", "How near in degrees to a virtual notch angle you have to be for it to snap to nearest notch");
//         }

//         if (ImGui::Checkbox("Stick Deadzones For Buttons", &profile->UseStickDeadzoneForButtons)) {
//             Context::GetInstance()->GetControlDeck()->SaveSettings();
//         }
//         if (ImGui::IsItemHovered()) {
//             ImGui::SetTooltip("%s", "Uses the stick deadzone values when sticks are mapped to buttons");
//         }
//     }

//     ImGui::Dummy(ImVec2(0, 5));
//     EndGroupPanel(isKeyboard ? 0.0f : 2.0f);
// }

void InputEditorWindow::UpdateElement() {
    if (ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId)) {
        LUS::Context::GetInstance()->GetControlDeck()->BlockGameInput();

        // continue to block input for a third of a second after getting the mapping
        mGameInputBlockTimer = ImGui::GetIO().Framerate / 3;
    } else {
        if (mGameInputBlockTimer != INT32_MAX) {
            mGameInputBlockTimer--;
            if (mGameInputBlockTimer <= 0) {
                LUS::Context::GetInstance()->GetControlDeck()->UnblockGameInput();
                mGameInputBlockTimer = INT32_MAX;
            }
        }
    }
}

void InputEditorWindow::DrawAnalogPreview(const char* label, ImVec2 stick, float deadzone) {
    ImGui::BeginChild(label, ImVec2(78, 85), false);
    ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPos().x + 10, ImGui::GetCursorPos().y + 10));
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    const ImVec2 cursorScreenPosition = ImGui::GetCursorScreenPos();

    // Draw the border box
    float borderSquareLeft = cursorScreenPosition.x + 2.0f;
    float borderSquareTop = cursorScreenPosition.y + 2.0f;
    float borderSquareSize = 65.0f;
    drawList->AddRect(ImVec2(borderSquareLeft, borderSquareTop),
                      ImVec2(borderSquareLeft + borderSquareSize, borderSquareTop + borderSquareSize),
                      ImColor(100, 100, 100, 255), 0.0f, 0, 1.5f);

    // Draw the gate background
    float cardinalRadius = 22.5f;
    float diagonalRadius = cardinalRadius * (69.0f / 85.0f);

    ImVec2 joystickCenterpoint =
        ImVec2(cursorScreenPosition.x + cardinalRadius + 12, cursorScreenPosition.y + cardinalRadius + 11);
    drawList->AddQuadFilled(joystickCenterpoint,
                            ImVec2(joystickCenterpoint.x - diagonalRadius, joystickCenterpoint.y + diagonalRadius),
                            ImVec2(joystickCenterpoint.x, joystickCenterpoint.y + cardinalRadius),
                            ImVec2(joystickCenterpoint.x + diagonalRadius, joystickCenterpoint.y + diagonalRadius),
                            ImColor(130, 130, 130, 255));
    drawList->AddQuadFilled(joystickCenterpoint,
                            ImVec2(joystickCenterpoint.x + diagonalRadius, joystickCenterpoint.y + diagonalRadius),
                            ImVec2(joystickCenterpoint.x + cardinalRadius, joystickCenterpoint.y),
                            ImVec2(joystickCenterpoint.x + diagonalRadius, joystickCenterpoint.y - diagonalRadius),
                            ImColor(130, 130, 130, 255));
    drawList->AddQuadFilled(joystickCenterpoint,
                            ImVec2(joystickCenterpoint.x + diagonalRadius, joystickCenterpoint.y - diagonalRadius),
                            ImVec2(joystickCenterpoint.x, joystickCenterpoint.y - cardinalRadius),
                            ImVec2(joystickCenterpoint.x - diagonalRadius, joystickCenterpoint.y - diagonalRadius),
                            ImColor(130, 130, 130, 255));
    drawList->AddQuadFilled(joystickCenterpoint,
                            ImVec2(joystickCenterpoint.x - diagonalRadius, joystickCenterpoint.y - diagonalRadius),
                            ImVec2(joystickCenterpoint.x - cardinalRadius, joystickCenterpoint.y),
                            ImVec2(joystickCenterpoint.x - diagonalRadius, joystickCenterpoint.y + diagonalRadius),
                            ImColor(130, 130, 130, 255));

    // Draw the joystick position indicator
    ImVec2 joystickIndicatorDistanceFromCenter = ImVec2(0, 0);
    if ((stick.x * stick.x + stick.y * stick.y) > (deadzone * deadzone)) {
        joystickIndicatorDistanceFromCenter =
            ImVec2((stick.x * (cardinalRadius / 85.0f)), -(stick.y * (cardinalRadius / 85.0f)));
    }
    float indicatorRadius = 5.0f;
    drawList->AddCircleFilled(ImVec2(joystickCenterpoint.x + joystickIndicatorDistanceFromCenter.x,
                                     joystickCenterpoint.y + joystickIndicatorDistanceFromCenter.y),
                              indicatorRadius, ImColor(34, 51, 76, 255), 7);

    ImGui::EndChild();
}

#define CHIP_COLOR_N64_GREY ImVec4(0.4f, 0.4f, 0.4f, 1.0f)
#define CHIP_COLOR_N64_BLUE ImVec4(0.176f, 0.176f, 0.5f, 1.0f)
#define CHIP_COLOR_N64_GREEN ImVec4(0.0f, 0.294f, 0.0f, 1.0f)
#define CHIP_COLOR_N64_YELLOW ImVec4(0.5f, 0.314f, 0.0f, 1.0f)
#define CHIP_COLOR_N64_RED ImVec4(0.392f, 0.0f, 0.0f, 1.0f)

void InputEditorWindow::DrawInputChip(const char* buttonName, ImVec4 color = CHIP_COLOR_N64_GREY) {
    ImGui::BeginDisabled();
    ImGui::PushStyleColor(ImGuiCol_Button, color);
    ImGui::Button(buttonName, ImVec2(50.0f, 0));
    ImGui::PopStyleColor();
    ImGui::EndDisabled();
}

void InputEditorWindow::DrawButtonLineAddMappingButton(uint8_t port, uint16_t bitmask) {
    ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(1.0f, 0.5f));
    if (ImGui::Button(StringHelper::Sprintf("%s###addButtonMappingButton%d-%d", ICON_FA_PLUS, port, bitmask).c_str(),
                      ImVec2(20.0f, 0.0f))) {
        ImGui::OpenPopup(StringHelper::Sprintf("addButtonMappingPopup##%d-%d", port, bitmask).c_str());
    };
    ImGui::PopStyleVar();

    if (ImGui::BeginPopup(StringHelper::Sprintf("addButtonMappingPopup##%d-%d", port, bitmask).c_str())) {
        ImGui::Text("Press any button,\nmove any axis,\nor press any key\nto add mapping");
        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
        }
        // todo: figure out why optional params (using uuid = "" in the definition) wasn't working
        if (LUS::Context::GetInstance()
                ->GetControlDeck()
                ->GetControllerByPort(port)
                ->GetButton(bitmask)
                ->AddOrEditButtonMappingFromRawPress(bitmask, "")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void InputEditorWindow::DrawButtonLineEditMappingButton(uint8_t port, uint16_t bitmask, std::string id) {
    auto mapping = LUS::Context::GetInstance()
                       ->GetControlDeck()
                       ->GetControllerByPort(port)
                       ->GetButton(bitmask)
                       ->GetButtonMappingById(id);
    if (mapping == nullptr) {
        return;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.5f));
    std::string icon = "";
    switch (mapping->GetMappingType()) {
        case MAPPING_TYPE_GAMEPAD:
            icon = ICON_FA_GAMEPAD;
            break;
        case MAPPING_TYPE_KEYBOARD:
            icon = ICON_FA_KEYBOARD_O;
            break;
        case MAPPING_TYPE_UNKNOWN:
            icon = ICON_FA_BUG;
            break;
    }
    if (ImGui::Button(StringHelper::Sprintf("%s %s ###editButtonMappingButton%s", icon.c_str(),
                                            mapping->GetPhysicalInputName().c_str(), id.c_str())
                          .c_str())) {
        ImGui::OpenPopup(StringHelper::Sprintf("editButtonMappingPopup##%s", id.c_str()).c_str());
    }

    if (ImGui::BeginPopup(StringHelper::Sprintf("editButtonMappingPopup##%s", id.c_str()).c_str())) {
        ImGui::Text("Press any button,\nmove any axis,\nor press any key\nto edit mapping");
        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
        }
        if (LUS::Context::GetInstance()
                ->GetControlDeck()
                ->GetControllerByPort(port)
                ->GetButton(bitmask)
                ->AddOrEditButtonMappingFromRawPress(bitmask, id)) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::PopStyleVar();
    ImGui::SameLine(0, 0);
    if (ImGui::Button(StringHelper::Sprintf("%s###removeButtonMappingButton%s", ICON_FA_TIMES, id.c_str()).c_str())) {
        LUS::Context::GetInstance()
            ->GetControlDeck()
            ->GetControllerByPort(port)
            ->GetButton(bitmask)
            ->ClearButtonMapping(id);
    };
    ImGui::SameLine(0, 4.0f);
}

void InputEditorWindow::DrawButtonLine(const char* buttonName, uint8_t port, uint16_t bitmask,
                                       ImVec4 color = CHIP_COLOR_N64_GREY) {
    ImGui::NewLine();
    ImGui::SameLine(32.0f);
    DrawInputChip(buttonName, color);
    ImGui::SameLine(86.0f);
    for (auto id : mBitmaskToMappingIds[port][bitmask]) {
        DrawButtonLineEditMappingButton(port, bitmask, id);
    }
    DrawButtonLineAddMappingButton(port, bitmask);
}

void InputEditorWindow::DrawStickDirectionLineAddMappingButton(uint8_t port, uint8_t stick, Direction direction) {
    ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(1.0f, 0.5f));
    if (ImGui::Button(
            StringHelper::Sprintf("%s###addStickDirectionMappingButton%d-%d-%d", ICON_FA_PLUS, port, stick, direction)
                .c_str(),
            ImVec2(20.0f, 0.0f))) {
        ImGui::OpenPopup(
            StringHelper::Sprintf("addStickDirectionMappingPopup##%d-%d-%d", port, stick, direction).c_str());
    };
    ImGui::PopStyleVar();

    if (ImGui::BeginPopup(
            StringHelper::Sprintf("addStickDirectionMappingPopup##%d-%d-%d", port, stick, direction).c_str())) {
        ImGui::Text("Press any button,\nmove any axis,\nor press any key\nto add mapping");
        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
        }
        if (stick == LEFT) {
            if (LUS::Context::GetInstance()
                    ->GetControlDeck()
                    ->GetControllerByPort(port)
                    ->GetLeftStick()
                    ->AddOrEditAxisDirectionMappingFromRawPress(direction, "")) {
                ImGui::CloseCurrentPopup();
            }
        } else {
            if (LUS::Context::GetInstance()
                    ->GetControlDeck()
                    ->GetControllerByPort(port)
                    ->GetRightStick()
                    ->AddOrEditAxisDirectionMappingFromRawPress(direction, "")) {
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndPopup();
    }
}

void InputEditorWindow::DrawStickDirectionLineEditMappingButton(uint8_t port, uint8_t stick, Direction direction,
                                                                std::string id) {
    std::shared_ptr<ControllerAxisDirectionMapping> mapping = nullptr;
    if (stick == LEFT) {
        mapping = LUS::Context::GetInstance()
                      ->GetControlDeck()
                      ->GetControllerByPort(port)
                      ->GetLeftStick()
                      ->GetAxisDirectionMappingById(direction, id);
    } else {
        mapping = LUS::Context::GetInstance()
                      ->GetControlDeck()
                      ->GetControllerByPort(port)
                      ->GetRightStick()
                      ->GetAxisDirectionMappingById(direction, id);
    }

    if (mapping == nullptr) {
        return;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.5f));
    std::string icon = "";
    switch (mapping->GetMappingType()) {
        case MAPPING_TYPE_GAMEPAD:
            icon = ICON_FA_GAMEPAD;
            break;
        case MAPPING_TYPE_KEYBOARD:
            icon = ICON_FA_KEYBOARD_O;
            break;
        case MAPPING_TYPE_UNKNOWN:
            icon = ICON_FA_BUG;
            break;
    }
    if (ImGui::Button(StringHelper::Sprintf("%s %s ###editStickDirectionMappingButton%s", icon.c_str(),
                                            mapping->GetPhysicalInputName().c_str(), id.c_str())
                          .c_str())) {
        ImGui::OpenPopup(StringHelper::Sprintf("editStickDirectionMappingPopup##%s", id.c_str()).c_str());
    }

    if (ImGui::BeginPopup(StringHelper::Sprintf("editStickDirectionMappingPopup##%s", id.c_str()).c_str())) {
        ImGui::Text("Press any button,\nmove any axis,\nor press any key\nto edit mapping");
        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
        }

        if (stick == LEFT) {
            if (LUS::Context::GetInstance()
                    ->GetControlDeck()
                    ->GetControllerByPort(port)
                    ->GetLeftStick()
                    ->AddOrEditAxisDirectionMappingFromRawPress(direction, id)) {
                ImGui::CloseCurrentPopup();
            }
        } else {
            if (LUS::Context::GetInstance()
                    ->GetControlDeck()
                    ->GetControllerByPort(port)
                    ->GetRightStick()
                    ->AddOrEditAxisDirectionMappingFromRawPress(direction, id)) {
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndPopup();
    }

    ImGui::PopStyleVar();
    ImGui::SameLine(0, 0);
    if (ImGui::Button(
            StringHelper::Sprintf("%s###removeStickDirectionMappingButton%s", ICON_FA_TIMES, id.c_str()).c_str())) {
        if (stick == LEFT) {
            LUS::Context::GetInstance()
                ->GetControlDeck()
                ->GetControllerByPort(port)
                ->GetLeftStick()
                ->ClearAxisDirectionMapping(direction, id);
        } else {
            LUS::Context::GetInstance()
                ->GetControlDeck()
                ->GetControllerByPort(port)
                ->GetRightStick()
                ->ClearAxisDirectionMapping(direction, id);
        }
    };
    ImGui::SameLine(0, 4.0f);
}

void InputEditorWindow::DrawStickDirectionLine(const char* axisDirectionName, uint8_t port, uint8_t stick,
                                               Direction direction, ImVec4 color = CHIP_COLOR_N64_GREY) {
    ImGui::NewLine();
    ImGui::SameLine();
    DrawInputChip(axisDirectionName, color);
    ImGui::SameLine(62.0f);
    for (auto id : mStickDirectionToMappingIds[port][stick][direction]) {
        DrawStickDirectionLineEditMappingButton(port, stick, direction, id);
    }
    DrawStickDirectionLineAddMappingButton(port, stick, direction);
}

void InputEditorWindow::DrawStickSection(uint8_t port, uint8_t stick, int32_t id,
                                         ImVec4 color = CHIP_COLOR_N64_GREY) {
    static int8_t x, y;
    std::shared_ptr<ControllerStick> controllerStick = nullptr;
    if (stick == LEFT) {
        controllerStick = LUS::Context::GetInstance()->GetControlDeck()->GetControllerByPort(port)->GetLeftStick();
    } else {
        controllerStick = LUS::Context::GetInstance()->GetControlDeck()->GetControllerByPort(port)->GetRightStick();
    }
    controllerStick->Process(x, y);
    DrawAnalogPreview(StringHelper::Sprintf("##AnalogPreview%d", id).c_str(), ImVec2(x, y));

    ImGui::SameLine();
    ImGui::BeginGroup();
    DrawStickDirectionLine(ICON_FA_ARROW_LEFT, port, stick, LEFT, color);
    DrawStickDirectionLine(ICON_FA_ARROW_RIGHT, port, stick, RIGHT, color);
    DrawStickDirectionLine(ICON_FA_ARROW_UP, port, stick, UP, color);
    DrawStickDirectionLine(ICON_FA_ARROW_DOWN, port, stick, DOWN, color);
    ImGui::EndGroup();
    if (ImGui::TreeNode(StringHelper::Sprintf("Analog Stick Options##%d", id).c_str())) {
        ImGui::Text("Deadzone:");
        ImGui::SetNextItemWidth(160.0f);

        int32_t deadzonePercentage = controllerStick->GetDeadzonePercentage();
        if (ImGui::SliderInt(StringHelper::Sprintf("##Deadzone%d", id).c_str(), &deadzonePercentage, 0, 100, "%d%%",
                             ImGuiSliderFlags_AlwaysClamp)) {
            controllerStick->SetDeadzone(deadzonePercentage);
        }

        ImGui::Text("Notch Snap Angle:");
        ImGui::SetNextItemWidth(160.0f);

        int32_t notchSnapAngle = controllerStick->GetNotchSnapAngle();
        if (ImGui::SliderInt(StringHelper::Sprintf("##NotchProximityThreshold%d", id).c_str(), &notchSnapAngle, 0,
                         45, "%d°", ImGuiSliderFlags_AlwaysClamp)) {
            controllerStick->SetNotchSnapAngle(notchSnapAngle);
        }
        ImGui::TreePop();
    }
}

void InputEditorWindow::UpdateBitmaskToMappingIds(uint8_t port) {
    // todo: do we need this now that ControllerButton exists?

    for (auto [bitmask, button] :
         LUS::Context::GetInstance()->GetControlDeck()->GetControllerByPort(port)->GetAllButtons()) {
        for (auto [id, mapping] : button->GetAllButtonMappings()) {
            // using a vector here instead of a set because i want newly added mappings
            // to go to the end of the list instead of autosorting
            if (std::find(mBitmaskToMappingIds[port][bitmask].begin(), mBitmaskToMappingIds[port][bitmask].end(), id) ==
                mBitmaskToMappingIds[port][bitmask].end()) {
                mBitmaskToMappingIds[port][bitmask].push_back(id);
            }
        }
    }
}

void InputEditorWindow::UpdateStickDirectionToMappingIds(uint8_t port) {
    // todo: do we need this?
    for (auto stick :
         { std::make_pair<uint8_t, std::shared_ptr<ControllerStick>>(
               LEFT, LUS::Context::GetInstance()->GetControlDeck()->GetControllerByPort(port)->GetLeftStick()),
           std::make_pair<uint8_t, std::shared_ptr<ControllerStick>>(
               RIGHT, LUS::Context::GetInstance()->GetControlDeck()->GetControllerByPort(port)->GetRightStick()) }) {
        for (auto direction : { LEFT, RIGHT, UP, DOWN }) {
            for (auto [id, mapping] : stick.second->GetAllAxisDirectionMappingByDirection(direction)) {
                // using a vector here instead of a set because i want newly added mappings
                // to go to the end of the list instead of autosorting
                if (std::find(mStickDirectionToMappingIds[port][stick.first][direction].begin(),
                              mStickDirectionToMappingIds[port][stick.first][direction].end(),
                              id) == mStickDirectionToMappingIds[port][stick.first][direction].end()) {
                    mStickDirectionToMappingIds[port][stick.first][direction].push_back(id);
                }
            }
        }
    }
}

void InputEditorWindow::DrawElement() {
    static bool connected[4] = { true, false, false, false };
    // static bool openTab[4] = {false, false, false, false};
    ImGui::Begin("Controller Configuration", &mIsVisible);
    ImGui::BeginTabBar("##ControllerConfigPortTabs");
    for (uint8_t i = 0; i < 4; i++) {
        if (ImGui::BeginTabItem(StringHelper::Sprintf("%s Port %d###port%d",
                                                      connected[i] ? ICON_FA_PLUG : ICON_FA_CHAIN_BROKEN, i + 1, i)
                                    .c_str())) {
            if (ImGui::Button("Clear All")) {
                LUS::Context::GetInstance()->GetControlDeck()->GetControllerByPort(i)->ClearAllMappings();
            }

            UpdateBitmaskToMappingIds(i);
            UpdateStickDirectionToMappingIds(i);

            if (ImGui::CollapsingHeader("Buttons", NULL, ImGuiTreeNodeFlags_DefaultOpen)) {
                DrawButtonLine("A", i, BTN_A, CHIP_COLOR_N64_BLUE);
                DrawButtonLine("B", i, BTN_B, CHIP_COLOR_N64_GREEN);
                DrawButtonLine("L", i, BTN_L);
                DrawButtonLine("R", i, BTN_R);
                DrawButtonLine("Z", i, BTN_Z);
                DrawButtonLine("Start", i, BTN_START, CHIP_COLOR_N64_RED);
            }
            if (ImGui::CollapsingHeader("C-Buttons", NULL, ImGuiTreeNodeFlags_DefaultOpen)) {
                DrawButtonLine(StringHelper::Sprintf("C %s", ICON_FA_ARROW_LEFT).c_str(), i, BTN_CLEFT,
                               CHIP_COLOR_N64_YELLOW);
                DrawButtonLine(StringHelper::Sprintf("C %s", ICON_FA_ARROW_RIGHT).c_str(), i, BTN_CRIGHT,
                               CHIP_COLOR_N64_YELLOW);
                DrawButtonLine(StringHelper::Sprintf("C %s", ICON_FA_ARROW_UP).c_str(), i, BTN_CUP,
                               CHIP_COLOR_N64_YELLOW);
                DrawButtonLine(StringHelper::Sprintf("C %s", ICON_FA_ARROW_DOWN).c_str(), i, BTN_CDOWN,
                               CHIP_COLOR_N64_YELLOW);
            }
            if (ImGui::CollapsingHeader("D-Pad", NULL, ImGuiTreeNodeFlags_DefaultOpen)) {
                DrawButtonLine(StringHelper::Sprintf("%s %s", ICON_FA_PLUS, ICON_FA_ARROW_LEFT).c_str(), i, BTN_DLEFT);
                DrawButtonLine(StringHelper::Sprintf("%s %s", ICON_FA_PLUS, ICON_FA_ARROW_RIGHT).c_str(), i,
                               BTN_DRIGHT);
                DrawButtonLine(StringHelper::Sprintf("%s %s", ICON_FA_PLUS, ICON_FA_ARROW_UP).c_str(), i, BTN_DUP);
                DrawButtonLine(StringHelper::Sprintf("%s %s", ICON_FA_PLUS, ICON_FA_ARROW_DOWN).c_str(), i, BTN_DDOWN);
            }
            if (ImGui::CollapsingHeader("Analog Stick", NULL, ImGuiTreeNodeFlags_DefaultOpen)) {
                static int32_t deadzone = 20;
                static int32_t notchProximityThreshold = 0;
                DrawStickSection(i, LEFT, 0);
            }
            if (ImGui::CollapsingHeader("Additional (\"Right\") Stick")) {
                static int32_t additionalDeadzone = 20;
                static int32_t additionalNotchProximityThreshold = 0;
                DrawStickSection(i, RIGHT, 1, CHIP_COLOR_N64_YELLOW);
            }
            if (ImGui::CollapsingHeader("Gyro")) {
                // does previewing using the analog stick preview work currently?
                // i'd think trying to normalize gyro to an analog stick would be problematic.
                // i'm thinking we might be better off just throwing a couple non-interactable
                // sliders in here so people can see gyro working (and we can throw numbers on them too)
                DrawAnalogPreview("##gyro", ImVec2(0.0f, 0.0f));
                ImGui::SameLine();
                ImGui::BeginGroup();
                ImGui::Text("Gyro Device");
                static int8_t selectedGyroController = -1;
                ImGui::SetNextItemWidth(160.0f);
                if (ImGui::BeginCombo("##gryoControllers", "None")) {
                    for (int8_t i = 0; i < 4; i++) {
                        std::string deviceName = StringHelper::Sprintf("Controller %d", i);
                        if (ImGui::Selectable(deviceName.c_str(), i == selectedGyroController)) {
                            selectedGyroController = i;
                        }
                    }
                    ImGui::EndCombo();
                }
                ImGui::Text("todo: either figure out sdl doesn't think my controllers have gyro\nor find someone else "
                            "to finish the gyro section");
                ImGui::EndGroup();
            }
            if (ImGui::CollapsingHeader("Attachments (Rumble etc.)")) {
                ImGui::Text("todo: for rumble it'd be ideal to be able to add motors instead of just controllers\nso "
                            "you could say do 10%% intensity on the big motor and 50%% on the small one");
                ImGui::Text("todo: leds");
            }
            ImGui::EndTabItem();
        }
    }
    ImGui::EndTabBar();
    ImGui::End();
}
} // namespace LUS
