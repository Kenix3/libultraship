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

/**
 * @brief Component wrapper around a BS::thread_pool.
 *
 * ThreadPoolComponent manages a thread pool within the component system,
 * providing convenience methods for pausing and resuming pool execution.
 */
class ThreadPoolComponent : public Component {
  public:
    /**
     * @brief Constructs a ThreadPoolComponent with the given number of worker threads.
     * @param threadCount The number of threads in the pool.
     */
    explicit ThreadPoolComponent(size_t threadCount);
    ~ThreadPoolComponent() override = default;

    /**
     * @brief Returns the underlying thread pool.
     * @return A shared pointer to the BS::thread_pool.
     */
    std::shared_ptr<BS::thread_pool> GetPool() const;

    /** @brief Pauses the thread pool, preventing it from picking up new tasks. */
    void Pause();

    /** @brief Unpauses the thread pool, allowing it to resume picking up tasks. */
    void Unpause();

  private:
    std::shared_ptr<BS::thread_pool> mThreadPool;
};

} // namespace Ship
