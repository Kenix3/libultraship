#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <functional>
#ifdef _WIN32
#include <windows.h>
#endif
#include <libusb.h>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>

// --- Minimal Logging (FIXED) ---
#define NOTICE_LOG_FMT(fmt) printf("NOTICE: " fmt "\n")
#define WARN_LOG_FMT(fmt) printf("WARN: " fmt "\n")
#define ERROR_LOG_FMT(fmt) printf("ERROR: " fmt "\n")
#define INFO_LOG_FMT(fmt) printf("INFO: " fmt "\n")

// --- Sync Primitives ---
namespace Common {
class Flag {
  public:
    // FIX: Added optional bool argument to match original code's usage
    void Set(bool value = true) {
        m_flag.store(value);
    }
    void Clear() {
        m_flag.store(false);
    }
    bool IsSet() const {
        return m_flag.load();
    }
    bool TestAndClear() {
        return m_flag.exchange(false);
    }

  private:
    std::atomic<bool> m_flag = { false };
};

class Event {
  public:
    void Wait() {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cond.wait(lock, [this] { return m_signaled; });
        m_signaled = false;
    }
    void Set() {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_signaled = true;
        }
        m_cond.notify_one();
    }

  private:
    std::mutex m_mutex;
    std::condition_variable m_cond;
    bool m_signaled = false;
};

inline void SetCurrentThreadName(const char* name) {
}
inline void SleepCurrentThread(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}
inline void YieldCPU() {
    std::this_thread::yield();
}

template <int bit, typename T> constexpr bool ExtractBit(T val) {
    return (val >> bit) & 1;
}

class ScopeGuard {
  public:
    ScopeGuard(std::function<void()> func) {
    }
    void Dismiss() {
    }
};
} // namespace Common

// --- Libusb Wrapper ---
namespace LibusbUtils {
class Context {
  public:
    Context() {
        libusb_init(&m_context);
    }
    ~Context() {
        libusb_exit(m_context);
    }
    operator libusb_context*() const {
        return m_context;
    }
    bool IsValid() const {
        return m_context != nullptr;
    }
    int GetDeviceList(std::function<bool(libusb_device*)> callback) {
        libusb_device** list;
        ssize_t count = libusb_get_device_list(m_context, &list);
        if (count < 0)
            return static_cast<int>(count);
        for (ssize_t i = 0; i < count; ++i) {
            if (!callback(list[i]))
                break;
        }
        libusb_free_device_list(list, 1);
        return 0;
    }

  private:
    libusb_context* m_context = nullptr;
};

using ConfigDescriptorPtr = std::unique_ptr<libusb_config_descriptor, decltype(&libusb_free_config_descriptor)>;
inline std::pair<int, ConfigDescriptorPtr> MakeConfigDescriptor(libusb_device* device) {
    libusb_config_descriptor* config = nullptr;
    const int error = libusb_get_active_config_descriptor(device, &config);
    return { error, ConfigDescriptorPtr(config, &libusb_free_config_descriptor) };
}
inline std::string ErrorWrap(int error_code) {
    return libusb_strerror(static_cast<libusb_error>(error_code));
}
} // namespace LibusbUtils

// --- Other Stubs ---
#define LIBUSB_DT_HID 0x21
namespace SerialInterface {
constexpr int MAX_SI_CHANNELS = 4;
enum SIDevices { SIDEVICE_WIIU_ADAPTER };
} // namespace SerialInterface
struct GCPadStatus {
    uint16_t button = 0;
    uint8_t stickX = 0, stickY = 0;
    uint8_t substickX = 0, substickY = 0;
    uint8_t triggerLeft = 0, triggerRight = 0;
};
#define PAD_BUTTON_A 0x0100
#define PAD_BUTTON_B 0x0200
#define PAD_BUTTON_X 0x0400
#define PAD_BUTTON_Y 0x0800
#define PAD_BUTTON_START 0x1000
#define PAD_BUTTON_LEFT 0x0001
#define PAD_BUTTON_RIGHT 0x0002
#define PAD_BUTTON_DOWN 0x0004
#define PAD_BUTTON_UP 0x0008
#define PAD_TRIGGER_Z 0x0010
#define PAD_TRIGGER_R 0x0020
#define PAD_TRIGGER_L 0x0040
#define PAD_GET_ORIGIN 0x2000
#define PAD_ERR_STATUS 0x4000

namespace Config {
using ConfigInfo = int;
using ConfigChangedCallbackID = int;
inline ConfigInfo GetInfoForSIDevice(int i) {
    return 0;
}
inline ConfigInfo GetInfoForAdapterRumble(int i) {
    return 0;
}
inline SerialInterface::SIDevices Get(ConfigInfo) {
    return SerialInterface::SIDEVICE_WIIU_ADAPTER;
}
inline bool Get(const ConfigInfo&, bool default_val) {
    return true;
}
inline ConfigChangedCallbackID AddConfigChangedCallback(std::function<void()>) {
    return 1;
}
inline void RemoveConfigChangedCallback(ConfigChangedCallbackID) {
}
} // namespace Config

namespace Core {
// FIX: Added 'Starting' to the enum
enum State { Uninitialized, Starting };
struct CoreTiming {
    uint64_t GetTicks() {
        return 0;
    }
};
struct SystemTimers {
    uint64_t GetTicksPerSecond() {
        return 1;
    }
};

class System {
  public:
    static System& GetInstance() {
        static System instance;
        return instance;
    }
    // FIX: Added missing methods
    CoreTiming& GetCoreTiming() {
        static CoreTiming t;
        return t;
    }
    SystemTimers& GetSystemTimers() {
        static SystemTimers t;
        return t;
    }
};

inline State GetState(System&) {
    return State::Uninitialized;
}
inline bool WantsDeterminism() {
    return false;
}
} // namespace Core