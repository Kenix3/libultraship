// #define NOMINMAX

// #include "SDLController.h"

// #include <spdlog/spdlog.h>
// #include <Utils/StringHelper.h>

// #ifdef _MSC_VER
// #define strdup _strdup
// #endif

// #define MAX_SDL_RANGE (float)INT16_MAX

// // NOLINTNEXTLINE
// auto format_as(SDL_GameControllerAxis a) {
//     return fmt::underlying(a);
// }

// namespace LUS {

// SDLController::SDLController(int32_t deviceIndex) : Controller(deviceIndex), mController(nullptr) {
// }

// bool SDLController::Open() {
//     const auto newCont = SDL_GameControllerOpen(mDeviceIndex);
//     const auto newContToo = SDL_GameControllerOpen(mDeviceIndex);

//     // We failed to load the controller. Go to next.
//     if (newCont == nullptr) {
//         SPDLOG_ERROR("SDL Controller failed to open: ({})", SDL_GetError());
//         return false;
//     }

//     mSupportsGyro = false;
//     if (SDL_GameControllerHasSensor(newCont, SDL_SENSOR_GYRO)) {
//         SDL_GameControllerSetSensorEnabled(newCont, SDL_SENSOR_GYRO, SDL_TRUE);
//         mSupportsGyro = true;
//     }

//     char guidBuf[33];
//     SDL_JoystickGetGUIDString(SDL_JoystickGetDeviceGUID(mDeviceIndex), guidBuf, sizeof(guidBuf));
//     mController = newCont;

// #ifdef __SWITCH__
//     mGuid = StringHelper::Sprintf("%s:%d", guidBuf, mDeviceIndex);
//     mControllerName = StringHelper::Sprintf("%s #%d", SDL_GameControllerNameForIndex(mDeviceIndex), mDeviceIndex +
//     1);
// #else
//     mGuid = std::string(guidBuf);
//     mControllerName = std::string(SDL_GameControllerNameForIndex(mDeviceIndex));
// #endif
//     return true;
// }

// bool SDLController::Close() {
//     if (mController != nullptr && SDL_WasInit(SDL_INIT_GAMECONTROLLER)) {
//         if (CanRumble()) {
//             SDL_GameControllerRumble(mController, 0, 0, 0);
//         }
//         SDL_GameControllerClose(mController);
//     }
//     mController = nullptr;

//     return true;
// }

// float SDLController::NormaliseStickValue(float axisValue) {
//     // scale {-32768 ... +32767} to {-MAX_AXIS_RANGE ... +MAX_AXIS_RANGE}
//     return axisValue * MAX_AXIS_RANGE / MAX_SDL_RANGE;
// }

// void SDLController::NormalizeStickAxis(SDL_GameControllerAxis axisX, SDL_GameControllerAxis axisY, int32_t portIndex)
// {
//     const auto axisValueX = SDL_GameControllerGetAxis(mController, axisX);
//     const auto axisValueY = SDL_GameControllerGetAxis(mController, axisY);

//     auto ax = NormaliseStickValue(axisValueX);
//     auto ay = NormaliseStickValue(axisValueY);

//     if (axisX == SDL_CONTROLLER_AXIS_LEFTX) {
//         GetLeftStickX(portIndex) = +ax;
//         GetLeftStickY(portIndex) = -ay;
//     } else if (axisX == SDL_CONTROLLER_AXIS_RIGHTX) {
//         GetRightStickX(portIndex) = +ax;
//         GetRightStickY(portIndex) = -ay;
//     }
// }

// int32_t SDLController::ReadRawPress() {
//     SDL_GameControllerUpdate();

//     for (int32_t i = SDL_CONTROLLER_BUTTON_A; i < SDL_CONTROLLER_BUTTON_MAX; i++) {
//         if (SDL_GameControllerGetButton(mController, static_cast<SDL_GameControllerButton>(i))) {
//             return i;
//         }
//     }

//     for (int32_t i = SDL_CONTROLLER_AXIS_LEFTX; i < SDL_CONTROLLER_AXIS_MAX; i++) {
//         const auto axis = static_cast<SDL_GameControllerAxis>(i);
//         const auto axisValue = SDL_GameControllerGetAxis(mController, axis) / 32767.0f;

//         if (axisValue < -0.7f) {
//             return -(axis + AXIS_SCANCODE_BIT);
//         }

//         if (axisValue > 0.7f) {
//             return (axis + AXIS_SCANCODE_BIT);
//         }
//     }

//     return -1;
// }

// int32_t SDLController::SetRumble(int32_t portIndex, bool rumble) {
//     if (!CanRumble()) {
//         return -1000;
//     }

//     if (!GetProfile(portIndex)->UseRumble) {
//         return -1001;
//     }

//     mIsRumbling = rumble;
//     if (mIsRumbling) {
//         float rumbleStrength = GetProfile(portIndex)->RumbleStrength;
//         return SDL_GameControllerRumble(mController, 0xFFFF * rumbleStrength, 0xFFFF * rumbleStrength, 0);
//     } else {
//         return SDL_GameControllerRumble(mController, 0, 0, 0);
//     }
// }

// int32_t SDLController::SetLedColor(int32_t portIndex, Color_RGB8 color) {
//     if (!CanSetLed()) {
//         return -1000;
//     }

//     mLedColor = color;
//     return SDL_JoystickSetLED(SDL_GameControllerGetJoystick(mController), mLedColor.r, mLedColor.g, mLedColor.b);
// }

// const std::string SDLController::GetButtonName(int32_t portIndex, int32_t n64bitmask) {
//     char buffer[50];
//     // OTRTODO: This should get the scancode of all bits in the mask.
//     std::map<int32_t, int32_t>& mappings = GetProfile(portIndex)->Mappings;

//     const auto find =
//         std::find_if(mappings.begin(), mappings.end(),
//                      [n64bitmask](const std::pair<int32_t, int32_t>& pair) { return pair.second == n64bitmask; });

//     if (find == mappings.end()) {
//         return "Unknown";
//     }

//     int btn = abs(find->first);

//     if (btn >= AXIS_SCANCODE_BIT) {
//         btn -= AXIS_SCANCODE_BIT;

//         snprintf(buffer, sizeof(buffer), "%s%s", sAxisNames[btn], find->first > 0 ? "+" : "-");
//         return buffer;
//     }

//     snprintf(buffer, sizeof(buffer), "Button %d", btn);
//     return buffer;
// }

// void SDLController::CreateDefaultBinding(int32_t portIndex) {
//     auto profile = GetProfile(portIndex);
//     profile->Mappings.clear();
//     profile->AxisDeadzones.clear();
//     profile->AxisMinimumPress.clear();
//     profile->GyroData.clear();

//     profile->Version = DEVICE_PROFILE_CURRENT_VERSION;
//     profile->UseRumble = true;
//     profile->RumbleStrength = 1.0f;
//     profile->UseGyro = false;

//     profile->Mappings[SDL_CONTROLLER_AXIS_RIGHTX | AXIS_SCANCODE_BIT] = BTN_CRIGHT;
//     profile->Mappings[-(SDL_CONTROLLER_AXIS_RIGHTX | AXIS_SCANCODE_BIT)] = BTN_CLEFT;
//     profile->Mappings[SDL_CONTROLLER_AXIS_RIGHTY | AXIS_SCANCODE_BIT] = BTN_CDOWN;
//     profile->Mappings[-(SDL_CONTROLLER_AXIS_RIGHTY | AXIS_SCANCODE_BIT)] = BTN_CUP;
//     profile->Mappings[SDL_CONTROLLER_AXIS_LEFTX | AXIS_SCANCODE_BIT] = BTN_STICKRIGHT;
//     profile->Mappings[-(SDL_CONTROLLER_AXIS_LEFTX | AXIS_SCANCODE_BIT)] = BTN_STICKLEFT;
//     profile->Mappings[SDL_CONTROLLER_AXIS_LEFTY | AXIS_SCANCODE_BIT] = BTN_STICKDOWN;
//     profile->Mappings[-(SDL_CONTROLLER_AXIS_LEFTY | AXIS_SCANCODE_BIT)] = BTN_STICKUP;
//     profile->Mappings[SDL_CONTROLLER_AXIS_TRIGGERRIGHT | AXIS_SCANCODE_BIT] = BTN_R;
//     profile->Mappings[SDL_CONTROLLER_AXIS_TRIGGERLEFT | AXIS_SCANCODE_BIT] = BTN_Z;
//     profile->Mappings[SDL_CONTROLLER_BUTTON_LEFTSHOULDER] = BTN_L;
//     profile->Mappings[SDL_CONTROLLER_BUTTON_DPAD_RIGHT] = BTN_DRIGHT;
//     profile->Mappings[SDL_CONTROLLER_BUTTON_DPAD_LEFT] = BTN_DLEFT;
//     profile->Mappings[SDL_CONTROLLER_BUTTON_DPAD_DOWN] = BTN_DDOWN;
//     profile->Mappings[SDL_CONTROLLER_BUTTON_DPAD_UP] = BTN_DUP;
//     profile->Mappings[SDL_CONTROLLER_BUTTON_START] = BTN_START;
//     profile->Mappings[SDL_CONTROLLER_BUTTON_B] = BTN_B;
//     profile->Mappings[SDL_CONTROLLER_BUTTON_A] = BTN_A;
//     profile->Mappings[SDL_CONTROLLER_BUTTON_LEFTSTICK] = BTN_MODIFIER1;
//     profile->Mappings[SDL_CONTROLLER_BUTTON_RIGHTSTICK] = BTN_MODIFIER2;

//     for (int32_t i = SDL_CONTROLLER_AXIS_LEFTX; i < SDL_CONTROLLER_AXIS_MAX; i++) {
//         profile->AxisDeadzones[i] = 16.0f;
//         profile->AxisMinimumPress[i] = 7680.0f;
//     }

//     profile->GyroData[DRIFT_X] = 0.0f;
//     profile->GyroData[DRIFT_Y] = 0.0f;
//     profile->GyroData[GYRO_SENSITIVITY] = 1.0f;
// }

// bool SDLController::Connected() const {
//     return mController != nullptr;
// }

// bool SDLController::CanGyro() const {
//     return mSupportsGyro;
// }

// bool SDLController::CanRumble() const {
// #if SDL_COMPILEDVERSION >= SDL_VERSIONNUM(2, 0, 18)
//     return SDL_GameControllerHasRumble(mController);
// #endif
//     return false;
// }

// bool SDLController::CanSetLed() const {
//     return SDL_GameControllerHasLED(mController);
// }

// void SDLController::ClearRawPress() {
// }
// } // namespace LUS
