#include "ship/Tickable.h"
#include "ship/Component.h"

#include "spdlog/spdlog.h"

namespace Ship {
Tickable::Tickable(bool isTicking, bool isDrawing)
    : mIsTicking(isTicking), mIsDrawing(isDrawing)
#ifdef INCLUDE_TICKABLE_MULTITHREAD
,     mMutex()
#endif
#ifdef INCLDUE_TICKABLE_PROFILING
,     mTickStartClock(std::chrono::high_resolution_clock::now()),
      mDrawStartClock(std::chrono::high_resolution_clock::now()),
      mTickEndClock(std::chrono::high_resolution_clock::now()),
      mDrawEndClock(std::chrono::high_resolution_clock::now()),
      mPreviousTickStartClock(std::chrono::high_resolution_clock::now()),
      mPreviousDrawStartClock(std::chrono::high_resolution_clock::now()),
      mPreviousTickEndClock(std::chrono::high_resolution_clock::now()),
      mPreviousDrawEndClock(std::chrono::high_resolution_clock::now())
#endif
{
}

Tickable::~Tickable() = default;

bool Tickable::Tick(const double durationSinceLastTick) {
#ifdef INCLDUE_TICKABLE_PROFILING
    // Set Previous clocks to the current clock.
    SetPreviousTickStartClock(GetTickStartClock());
    SetPreviousTickEndClock(GetTickEndClock());

    // Set Start clock to now, and end clocks to "empty"
    // Then run the logic. After the logic, set the end time.
    SetTickStartClock(std::chrono::high_resolution_clock::now());
    SetTickEndClock(std::chrono::time_point<std::chrono::steady_clock>(std::chrono::system_clock::duration::zero()));
#endif

    bool val = true;
    if (IsTicking()) {
        val = Ticked(durationSinceLastTick);
    }

#ifdef INCLDUE_TICKABLE_PROFILING
    SetTickEndClock(std::chrono::high_resolution_clock::now());
#endif

    return val;
}

bool Tickable::Draw(const double durationSinceLastDraw) {
#ifdef INCLDUE_TICKABLE_PROFILING
        // Set Previous clocks to the current clock.
        SetPreviousDrawStartClock(GetDrawStartClock());
        SetPreviousDrawEndClock(GetDrawEndClock());

        // Set Start clock to now, and end clocks to "empty"
        // Then run the logic. After the logic, set the end time.
        SetDrawStartClock(std::chrono::high_resolution_clock::now());
        SetDrawEndClock(
            std::chrono::time_point<std::chrono::steady_clock>(std::chrono::system_clock::duration::zero()));
#endif

    bool val = true;
    if (IsDrawing()) {
        val = Drawn(durationSinceLastDraw);
    }

#ifdef INCLDUE_TICKABLE_PROFILING
        SetDrawEndClock(std::chrono::high_resolution_clock::now());
#endif

    return val;
}

bool Tickable::IsTicking() {
#ifdef INCLUDE_TICKABLE_MULTITHREAD
    const std::lock_guard<std::mutex> lock(mMutex);
#endif
    return mIsTicking;
}

bool Tickable::IsDrawing() {
#ifdef INCLUDE_TICKABLE_MULTITHREAD
    const std::lock_guard<std::mutex> lock(mMutex);
#endif
    return mIsDrawing;
}

bool Tickable::StartTicking(bool force) {
#ifdef INCLUDE_TICKABLE_MULTITHREAD
    const std::lock_guard<std::mutex> lock(mMutex);
#endif

    if (IsTicking()) {
        return true;
    }

    bool success = TickingStarted(force);
    if (force || success) {
        mIsTicking = true;

        if (force && !success) {
            // If this is also a component, log out.
            auto component = dynamic_cast<Component*>(this);
            if (component != nullptr) {
                auto string = component->ToString();
                SPDLOG_WARN("Forcing Tick Start on Component {}", string);
            }
        } else if (!force && !success) {
            return false;
        }
    }

    return true;
}

bool Tickable::StartDrawing(bool force) {
#ifdef INCLUDE_TICKABLE_MULTITHREAD
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
#endif

    if (IsDrawing()) {
        return true;
    }

    bool success = DrawingStarted(force);
    if (force || success) {
        mIsDrawing = true;

        if (force && !success) {
            // If this is also a component, log out.
            auto component = dynamic_cast<Component*>(this);
            if (component != nullptr) {
                auto string = component->ToString();
                SPDLOG_WARN("Forcing Draw Start on Component {}", string);
            }
        } else if (!force && !success) {
            return false;
        }
    }

    return true;
}

bool Tickable::StopTicking(bool force) {
#ifdef INCLUDE_TICKABLE_MULTITHREAD
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
#endif

    if (!IsTicking()) {
        return true;
    }

    bool success = TickingStopped(force);
    if (force || success) {
        mIsTicking = false;

        if (force && !success) {
            // If this is also a component, log out.
            auto component = dynamic_cast<Component*>(this);
            if (component != nullptr) {
                auto string = component->ToString();
                SPDLOG_WARN("Forcing Tick Stop on Component {}", string);
            }
        } else if (!force && !success) {
            return false;
        }
    }

    return true;
}

bool Tickable::StopDrawing(bool force) {
#ifdef INCLUDE_TICKABLE_MULTITHREAD
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
#endif

    if (!IsDrawing()) {
        return true;
    }

    bool success = DrawingStopped(force);
    if (force || success) {
        mIsDrawing = false;

        if (force && !success) {
            // If this is also a component, log out.
            auto component = dynamic_cast<Component*>(this);
            if (component != nullptr) {
                auto string = component->ToString();
                SPDLOG_WARN("Forcing Draw Stop on Component {}", string);
            }
        } else if (!force && !success) {
            return false;
        }
    }

    return true;
}

#ifdef INCLDUE_TICKABLE_PROFILING
double Tickable::GetTickStartTime() const {
    return std::chrono::duration<double>(GetTickStartClock().time_since_epoch()).count();
}

double Tickable::GetDrawStartTime() const {
    return std::chrono::duration<double>(GetDrawStartClock().time_since_epoch()).count();
}

double Tickable::GetTickEndTime() const {
    return std::chrono::duration<double>(GetTickEndClock().time_since_epoch()).count();
}

double Tickable::GetDrawEndTime() const {
    return std::chrono::duration<double>(GetDrawEndClock().time_since_epoch()).count();
}

double Tickable::GetPreviousTickStartTime() const {
    return std::chrono::duration<double>(GetPreviousTickStartClock().time_since_epoch()).count();
}

double Tickable::GetPreviousDrawStartTime() const {
    return std::chrono::duration<double>(GetPreviousDrawStartClock().time_since_epoch()).count();
}

double Tickable::GetPreviousTickEndTime() const {
    return std::chrono::duration<double>(GetPreviousTickEndClock().time_since_epoch()).count();
}
double Tickable::GetPreviousDrawEndTime() const {
    return std::chrono::duration<double>(GetPreviousDrawEndClock().time_since_epoch()).count();
}

double Tickable::GetDurationSinceLastTick() const {
    return std::chrono::duration<double>(GetTickStartClock() - GetPreviousTickStartClock()).count();
}

double Tickable::GetPreviousTickDuration() const {
    return std::chrono::duration<double>(GetPreviousTickStartClock() - GetPreviousTickEndClock()).count();
}

double Tickable::GetPreviousDrawDuration() const {
    return std::chrono::duration<double>(GetPreviousDrawStartClock() - GetPreviousDrawEndClock()).count();
}

std::chrono::time_point<std::chrono::steady_clock> Tickable::GetTickStartClock() const {
    return mTickStartClock;
}

std::chrono::time_point<std::chrono::steady_clock> Tickable::GetDrawStartClock() const {
    return mDrawStartClock;
}

std::chrono::time_point<std::chrono::steady_clock> Tickable::GetTickEndClock() const {
    return mTickEndClock;
}

std::chrono::time_point<std::chrono::steady_clock> Tickable::GetDrawEndClock() const {
    return mDrawEndClock;
}

std::chrono::time_point<std::chrono::steady_clock> Tickable::GetPreviousTickStartClock() const {
    return mPreviousTickStartClock;
}

std::chrono::time_point<std::chrono::steady_clock> Tickable::GetPreviousDrawStartClock() const {
    return mPreviousDrawStartClock;
}

std::chrono::time_point<std::chrono::steady_clock> Tickable::GetPreviousTickEndClock() const {
    return mPreviousTickEndClock;
}

std::chrono::time_point<std::chrono::steady_clock> Tickable::GetPreviousDrawEndClock() const {
    return mPreviousDrawEndClock;
}

Tickable& Tickable::SetTickStartClock(std::chrono::time_point<std::chrono::steady_clock> updateStartClock) {
    mTickStartClock = updateStartClock;
    return *this;
}

Tickable& Tickable::SetDrawStartClock(std::chrono::time_point<std::chrono::steady_clock> drawStartClock) {
    mDrawStartClock = drawStartClock;
    return *this;
}

Tickable& Tickable::SetTickEndClock(std::chrono::time_point<std::chrono::steady_clock> updateEndClock) {
    mTickEndClock = updateEndClock;
    return *this;
}

Tickable& Tickable::SetDrawEndClock(std::chrono::time_point<std::chrono::steady_clock> drawEndClock) {
    mDrawEndClock = drawEndClock;
    return *this;
}

Tickable&
Tickable::SetPreviousTickStartClock(std::chrono::time_point<std::chrono::steady_clock> previousTickStartClock) {
    mPreviousTickStartClock = previousTickStartClock;
    return *this;
}

Tickable&
Tickable::SetPreviousDrawStartClock(std::chrono::time_point<std::chrono::steady_clock> previousDrawStartClock) {
    mPreviousDrawStartClock = previousDrawStartClock;
    return *this;
}

Tickable&
Tickable::SetPreviousTickEndClock(std::chrono::time_point<std::chrono::steady_clock> previousTickEndClock) {
    mPreviousTickEndClock = previousTickEndClock;
    return *this;
}

Tickable&
Tickable::SetPreviousDrawEndClock(std::chrono::time_point<std::chrono::steady_clock> previousTickEndClock) {
    mPreviousDrawEndClock = previousTickEndClock;
    return *this;
}
#endif
} // namespace Ship
