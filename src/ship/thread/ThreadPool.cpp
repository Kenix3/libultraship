#include "ship/thread/ThreadPool.h"

namespace Ship {

ThreadPool::ThreadPool(size_t threadCount)
    : Component("ThreadPool"), mThreadPool(std::make_shared<BS::thread_pool>(threadCount)) {
    MarkInitialized();
}

std::shared_ptr<BS::thread_pool> ThreadPool::Get() const {
    return mThreadPool;
}

void ThreadPool::Pause() {
    mThreadPool->pause();
}

void ThreadPool::Unpause() {
    mThreadPool->unpause();
}

} // namespace Ship
