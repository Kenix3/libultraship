#pragma once

#include "ship/bridge/Bridge.h"

namespace LUS {
class UltraBridge : public Ship::Bridge {
  public:
    UltraBridge();
    ~UltraBridge() override;

    void UpdateCaches(const std::shared_ptr<Ship::Context>& context) override;
    void ClearCaches() override;
};
} // namespace LUS
