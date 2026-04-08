#include "ship/Action.h"
#include "ship/Tickable.h"

namespace Ship {
std::atomic_uint Action::NextActionId = 0;

Action::Action(const uint32_t actionType, std::shared_ptr<Tickable> tickable)
    : mActionId(NextActionId++), mActionType(actionType), mTickable(tickable), mIsActionRunning(false)
#ifdef INCLUDE_PROFILING
, mClocks()
#endif
{
}


} // namespace Ship