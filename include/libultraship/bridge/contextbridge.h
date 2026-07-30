#pragma once

#ifdef __cplusplus
#include <memory>

namespace Ship {
class Context;
}

void ContextBridgeInstallDefaultComponents(const std::shared_ptr<Ship::Context>& context);
void ContextBridgeUpdateCaches(const std::shared_ptr<Ship::Context>& context);
void ContextBridgeClearCaches();
void ContextBridgeRegisterCallbacks();
#endif
