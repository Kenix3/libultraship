// Copyright 2014 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "GCAdapter.h"

#include <algorithm>
#include <array>
#include <mutex>
#include <optional>
#include <libusb.h>
#include "GCAdapterStubs.h"

namespace GCAdapter {
constexpr unsigned int USB_TIMEOUT_MS = 100;

static bool CheckDeviceAccess(libusb_device* device);
static void AddGCAdapter(libusb_device* device);
static void Reset();
static void Setup();
static void ProcessInputPayload(const uint8_t* data, std::size_t size);
static void ReadThreadFunc();

enum class AdapterStatus {
    NotDetected,
    Detected,
    Error,
};

static std::atomic<AdapterStatus> s_status = AdapterStatus::NotDetected;
static std::atomic<libusb_error> s_adapter_error = LIBUSB_SUCCESS;
static libusb_device_handle* s_handle = nullptr;

enum class ControllerType : uint8_t {
    None = 0,
    Wired = 1,
    Wireless = 2,
};

constexpr size_t CONTROLLER_INPUT_PAYLOAD_EXPECTED_SIZE = 37;
constexpr size_t CONTROLLER_OUTPUT_INIT_PAYLOAD_SIZE = 1;

struct PortState {
    GCPadStatus status = {};
    ControllerType controller_type = ControllerType::None;
};

// State for the 4 controller ports
static std::array<PortState, SerialInterface::MAX_SI_CHANNELS> s_port_states;

// Threads and synchronization for reading input
static std::thread s_read_adapter_thread;
static Common::Flag s_read_adapter_thread_running;
static std::mutex s_read_mutex;
static std::mutex s_init_mutex;

// Thread for detecting the adapter
static std::thread s_adapter_detect_thread;
static Common::Flag s_adapter_detect_thread_running;

static std::unique_ptr<LibusbUtils::Context> s_libusb_context;
static uint8_t s_endpoint_in = 0;
static uint8_t s_endpoint_out = 0;

static void ReadThreadFunc() {
    Common::SetCurrentThreadName("GCAdapter Read Thread");
    NOTICE_LOG_FMT("GCAdapter read thread started");

    while (s_read_adapter_thread_running.IsSet()) {
        std::array<uint8_t, CONTROLLER_INPUT_PAYLOAD_EXPECTED_SIZE> input_buffer;
        int payload_size = 0;
        int error = libusb_interrupt_transfer(s_handle, s_endpoint_in, input_buffer.data(),
                                              static_cast<int>(input_buffer.size()), &payload_size, USB_TIMEOUT_MS);
        if (error != LIBUSB_SUCCESS && error != LIBUSB_ERROR_TIMEOUT) {
            ERROR_LOG_FMT("Read: libusb_interrupt_transfer failed");
        }
        if (error == LIBUSB_ERROR_IO) {
            libusb_reset_device(s_handle);
            ERROR_LOG_FMT("Read: libusb_reset_device called due to I/O error");
        }

        ProcessInputPayload(input_buffer.data(), payload_size);
        Common::YieldCPU();
    }

    NOTICE_LOG_FMT("GCAdapter read thread stopped");
}

static void ScanThreadFunc() {
    Common::SetCurrentThreadName("GC Adapter Scanning Thread");
    NOTICE_LOG_FMT("GC Adapter scanning thread started");

    while (s_adapter_detect_thread_running.IsSet()) {
        if (s_handle == nullptr) {
            std::lock_guard lk(s_init_mutex);
            Setup();
        }
        // Simple polling sleep instead of complex hotplug logic
        Common::SleepCurrentThread(500);
    }
    NOTICE_LOG_FMT("GC Adapter scanning thread stopped");
}

void Init() {
    if (s_handle != nullptr)
        return;

    s_libusb_context = std::make_unique<LibusbUtils::Context>();
    s_status = AdapterStatus::NotDetected;
    s_adapter_error = LIBUSB_SUCCESS;

    StartScanThread();
}

void StartScanThread() {
    if (s_adapter_detect_thread_running.IsSet() || !s_libusb_context->IsValid())
        return;

    s_adapter_detect_thread_running.Set();
    s_adapter_detect_thread = std::thread(ScanThreadFunc);
}

void StopScanThread() {
    if (s_adapter_detect_thread_running.TestAndClear()) {
        s_adapter_detect_thread.join();
    }
}

static void Setup() {
    // Reset the error status in case the adapter gets unplugged
    if (s_status == AdapterStatus::Error)
        s_status = AdapterStatus::NotDetected;

    s_port_states.fill({});

    s_libusb_context->GetDeviceList([](libusb_device* device) {
        if (CheckDeviceAccess(device)) {
            AddGCAdapter(device);
            return false; // Stop searching once we find one adapter
        }
        return true; // Keep searching
    });
}

static bool CheckDeviceAccess(libusb_device* device) {
    libusb_device_descriptor desc;
    if (libusb_get_device_descriptor(device, &desc) != LIBUSB_SUCCESS)
        return false;

    if (desc.idVendor != 0x057e || desc.idProduct != 0x0337)
        return false;

    NOTICE_LOG_FMT("Found GC Adapter");

    if (libusb_open(device, &s_handle) != LIBUSB_SUCCESS) {
        s_status = AdapterStatus::Error;
        ERROR_LOG_FMT("libusb_open failed to open device");
        return false;
    }

    if (libusb_kernel_driver_active(s_handle, 0) == 1) {
        // On macOS, we don't detach the kernel driver.
        // On other systems, you might need libusb_detach_kernel_driver.
    }

    // This call makes some third-party adapters work.
    libusb_control_transfer(s_handle, 0x21, 11, 0x0001, 0, nullptr, 0, 1000);

    if (libusb_claim_interface(s_handle, 0) != LIBUSB_SUCCESS) {
        ERROR_LOG_FMT("libusb_claim_interface failed");
        libusb_close(s_handle);
        s_handle = nullptr;
        return false;
    }

    return true;
}

static void AddGCAdapter(libusb_device* device) {
    auto [error, config] = LibusbUtils::MakeConfigDescriptor(device);
    if (error != LIBUSB_SUCCESS) {
        WARN_LOG_FMT("libusb_get_config_descriptor failed");
    }
    const libusb_interface* iface = &config->interface[0];
    const libusb_interface_descriptor* iface_desc = &iface->altsetting[0];
    for (uint8_t e = 0; e < iface_desc->bNumEndpoints; e++) {
        const libusb_endpoint_descriptor* endpoint = &iface_desc->endpoint[e];
        if (endpoint->bEndpointAddress & LIBUSB_ENDPOINT_IN)
            s_endpoint_in = endpoint->bEndpointAddress;
        else
            s_endpoint_out = endpoint->bEndpointAddress;
    }

    // Send initialization command to the adapter
    std::array<uint8_t, CONTROLLER_OUTPUT_INIT_PAYLOAD_SIZE> payload = { 0x13 };
    int size = 0;
    libusb_interrupt_transfer(s_handle, s_endpoint_out, payload.data(), CONTROLLER_OUTPUT_INIT_PAYLOAD_SIZE, &size,
                              USB_TIMEOUT_MS);

    s_read_adapter_thread_running.Set();
    s_read_adapter_thread = std::thread(ReadThreadFunc);

    s_status = AdapterStatus::Detected;
    NOTICE_LOG_FMT("GC Adapter attached and read thread started");
}

void Shutdown() {
    StopScanThread();
    Reset();
    s_libusb_context.reset();
}

static void Reset() {
    std::lock_guard lk(s_init_mutex);
    if (s_status != AdapterStatus::Detected)
        return;

    if (s_read_adapter_thread_running.TestAndClear())
        s_read_adapter_thread.join();

    s_port_states.fill({});
    s_status = AdapterStatus::NotDetected;

    if (s_handle) {
        libusb_release_interface(s_handle, 0);
        libusb_close(s_handle);
        s_handle = nullptr;
    }
    NOTICE_LOG_FMT("GC Adapter detached");
}

GCPadStatus Input(int chan) {
    if (s_handle == nullptr || s_status != AdapterStatus::Detected || chan >= SerialInterface::MAX_SI_CHANNELS)
        return {};

    std::lock_guard lk(s_read_mutex);
    return s_port_states[chan].status;
}

static ControllerType IdentifyControllerType(uint8_t data) {
    if (Common::ExtractBit<4>(data))
        return ControllerType::Wired;
    if (Common::ExtractBit<5>(data))
        return ControllerType::Wireless;
    return ControllerType::None;
}

void ProcessInputPayload(const uint8_t* data, std::size_t size) {
    if (size != CONTROLLER_INPUT_PAYLOAD_EXPECTED_SIZE || data[0] != LIBUSB_DT_HID) {
        // This can happen briefly on init, so we don't spam errors.
        return;
    }

    std::lock_guard lk(s_read_mutex);

    for (int chan = 0; chan != SerialInterface::MAX_SI_CHANNELS; ++chan) {
        const uint8_t* const channel_data = &data[1 + (9 * chan)];
        const auto type = IdentifyControllerType(channel_data[0]);
        auto& pad_state = s_port_states[chan];

        GCPadStatus pad = {};
        if (type != ControllerType::None) {
            const uint8_t b1 = channel_data[1];
            const uint8_t b2 = channel_data[2];

            if (Common::ExtractBit<0>(b1))
                pad.button |= PAD_BUTTON_A;
            if (Common::ExtractBit<1>(b1))
                pad.button |= PAD_BUTTON_B;
            if (Common::ExtractBit<2>(b1))
                pad.button |= PAD_BUTTON_X;
            if (Common::ExtractBit<3>(b1))
                pad.button |= PAD_BUTTON_Y;
            if (Common::ExtractBit<4>(b1))
                pad.button |= PAD_BUTTON_LEFT;
            if (Common::ExtractBit<5>(b1))
                pad.button |= PAD_BUTTON_RIGHT;
            if (Common::ExtractBit<6>(b1))
                pad.button |= PAD_BUTTON_DOWN;
            if (Common::ExtractBit<7>(b1))
                pad.button |= PAD_BUTTON_UP;
            if (Common::ExtractBit<0>(b2))
                pad.button |= PAD_BUTTON_START;
            if (Common::ExtractBit<1>(b2))
                pad.button |= PAD_TRIGGER_Z;
            if (Common::ExtractBit<2>(b2))
                pad.button |= PAD_TRIGGER_R;
            if (Common::ExtractBit<3>(b2))
                pad.button |= PAD_TRIGGER_L;

            pad.stickX = channel_data[3];
            pad.stickY = channel_data[4];
            pad.substickX = channel_data[5];
            pad.substickY = channel_data[6];
            pad.triggerLeft = channel_data[7];
            pad.triggerRight = channel_data[8];

        } else {
            pad.button = PAD_ERR_STATUS;
        }

        pad_state.controller_type = type;
        pad_state.status = pad;
    }
}

bool IsDetected(const char** error_message) {
    if (s_status != AdapterStatus::Error) {
        if (error_message)
            *error_message = nullptr;
        return s_status == AdapterStatus::Detected;
    }
    if (error_message)
        *error_message = libusb_strerror(s_adapter_error.load());
    return false;
}

} // namespace GCAdapter