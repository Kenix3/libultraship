#pragma once

#include <memory>
#include <string>
#include <thread>
#include <stdint.h>
#include "ship/Component.h"

#define BS_THREAD_POOL_ENABLE_PRIORITY
#define BS_THREAD_POOL_ENABLE_PAUSE
#include <BS_thread_pool.hpp>

namespace Ship {

class ThreadPoolComponent : public Component {
  public:
    explicit ThreadPoolComponent(size_t threadCount);
    ~ThreadPoolComponent() override = default;

    std::shared_ptr<BS::thread_pool> GetPool() const;

    void Pause();
    void Unpause();

  private:
    std::shared_ptr<BS::thread_pool> mThreadPool;
};

} // namespace Ship
