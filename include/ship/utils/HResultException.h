#pragma once

#ifdef _WIN32
#include <stdexcept>
#include <string>
#include <cstdio>
#include <windows.h>

namespace Ship {

/**
 * @brief Exception type for failed Windows HRESULT values.
 *
 * Replaces raw `throw hr;` patterns so that HRESULT failures are catchable
 * via the standard `catch (const std::exception&)` hierarchy.
 */
class HResultException : public std::runtime_error {
  public:
    explicit HResultException(HRESULT hr) : std::runtime_error(FormatMessage(hr)), mHResult(hr) {
    }

    HResultException(HRESULT hr, const std::string& context)
        : std::runtime_error(context + " " + FormatMessage(hr)), mHResult(hr) {
    }

    /** @brief Returns the raw HRESULT value. */
    HRESULT GetHResult() const noexcept {
        return mHResult;
    }

  private:
    HRESULT mHResult;

    static std::string FormatMessage(HRESULT hr) {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "HRESULT failure: 0x%08lX", static_cast<unsigned long>(hr));
        return buf;
    }
};

} // namespace Ship

#endif
