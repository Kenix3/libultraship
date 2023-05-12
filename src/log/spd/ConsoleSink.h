// Copyright(c) 2015-present, Gabi Melman & spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

#include <spdlog/details/fmt_helper.h>
#include <spdlog/details/null_mutex.h>
#include <spdlog/details/os.h>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/details/synchronous_factory.h>
#include "core/Context.h"
#include "gui/Gui.h"
#include "gui/ConsoleWindow.h"
#include <chrono>
#include <mutex>
#include <string>
#include <thread>
#include "core/bridge/consolevariablebridge.h"

namespace spdlog {
namespace sinks {

template <typename Mutex> class ConsoleSink final : public base_sink<Mutex> {
  public:
    explicit ConsoleSink(std::string tag = "spdlog", bool use_raw_msg = false)
        : tag_(std::move(tag)), use_raw_msg_(use_raw_msg) {
    }

  protected:
    void sink_it_(const details::log_msg& msg) override {
        memory_buf_t formatted;
        if (use_raw_msg_) {
            details::fmt_helper::append_string_view(msg.payload, formatted);
        } else {
            base_sink<Mutex>::formatter_->format(msg, formatted);
        }
        formatted.push_back('\0');
        const char* msg_output = formatted.data();
        if (CVarGetInteger("gSinkEnabled", 0) &&
            LUS::Context::GetInstance()->GetWindow()->GetGui()->GetGuiWindow("Console")->IsOpen()) {
            std::reinterpret_pointer_cast<LUS::ConsoleWindow>(
                LUS::Context::GetInstance()->GetWindow()->GetGui()->GetGuiWindow("Console"))
                ->Append("Logs", msg.level, "%s", msg_output);
        }
    }

    void flush_() override {
    }

  private:
    std::string tag_;
    bool use_raw_msg_;
};

using lus_sink_mt = ConsoleSink<std::mutex>;
using lus_sink_st = ConsoleSink<details::null_mutex>;
} // namespace sinks
} // namespace spdlog
