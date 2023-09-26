#pragma once

#include <cstdint>

namespace LUS {
class ControlDevice {
  public:
    ControlDevice(uint8_t portIndex);
    virtual ~ControlDevice();

  protected:
    uint8_t mPortIndex;

};
}


// have ref to port (i think we'd have header problem doing this because port includes this to keep a pointer to control device)
// just keeping the port index for now

// connected/disconnected (talked to mannus and decided against the checkbox in the ui, so we probably don't need this)
// have ref to attachmentport