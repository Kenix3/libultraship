#pragma once

#include <memory>

#include "ship/core/Component.h"

namespace Ship {
class Context;

class Bridge : public Component {
  public:
    using Component::Component;
    ~Bridge() override = default;

    virtual void UpdateCaches(const std::shared_ptr<Context>& context) = 0;
    virtual void ClearCaches() = 0;
};
} // namespace Ship
