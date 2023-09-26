// #pragma once

// #include "Controller.h"
// #include <vector>
// #include <config/Config.h>

// namespace LUS {

// class ControlPort {
//   public:
//     ControlDeck();
//     ~ControlDeck();

//     void Init(uint8_t* controllerBits);
//     void WriteToPad(OSContPad* pad);
//     OSContPad* GetPads();
//     uint8_t* GetControllerBits();
//     std::shared_ptr<Controller> GetControllerByPort(uint8_t port);
//     void BlockGameInput();
//     void UnblockGameInput();

//   private:
//     std::vector<std::shared_ptr<Controller>> mControllerPorts = {};
//     uint8_t* mControllerBits = nullptr;
//     OSContPad* mPads;
//     bool mGameInputBlocked;
// };
// } // namespace LUS

// has a control device
// has a port index
// connect/disconnect methods that update control device pointer