#pragma once

#include <functional>
#include "libultraship/libultra/controller.h"

#define DEFINE_HOOK(name, type)         \
    struct name {                       \
        typedef std::function<type> fn; \
    }

namespace LUS {
class Controller;

template <typename H> struct RegisteredHooks { inline static std::vector<typename H::fn> functions; };

template <typename H> void RegisterHook(typename H::fn h) {
    RegisteredHooks<H>::functions.push_back(h);
}

template <typename H, typename... Args> void ExecuteHooks(Args&&... args) {
    for (auto& fn : RegisteredHooks<H>::functions) {
        fn(std::forward<Args>(args)...);
    }
}

// SoH specific
DEFINE_HOOK(LoadFile, void(uint32_t fileNum));
DEFINE_HOOK(DeleteFile, void(uint32_t fileNum));
} // namespace LUS
