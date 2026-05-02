#include "ship/thread/ThreadPoolComponent.h"

namespace Ship {

ThreadPoolComponent::ThreadPoolComponent(size_t threadCount)
    : Component("ThreadPoolComponent"), mThreadPool(std::make_shared<BS::thread_pool>(threadCount)) {
    MarkInitialized();
}

std::shared_ptr<BS::thread_pool> ThreadPoolComponent::GetPool() const {
    return mThreadPool;
}

void ThreadPoolComponent::Pause() {
    mThreadPool->pause();
}

void ThreadPoolComponent::Unpause() {
    mThreadPool->unpause();
}

} // namespace Ship
