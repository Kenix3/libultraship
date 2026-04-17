#pragma once

#include <string>
#include <memory>
#include <vector>
#include <algorithm>

#ifdef COMPONENT_THREAD_SAFE
#include <shared_mutex>
#endif

#include "ship/Part.h"
#include "ship/PartList.h"

namespace Ship {

/**
 * @brief A named Part with a parent/child hierarchy and optional thread safety.
 *
 * Component extends Part with a human-readable name, a string representation,
 * and bidirectional parent/child relationships managed via PartList. When the
 * COMPONENT_THREAD_SAFE preprocessor flag is defined, all relationship
 * mutations are guarded by a shared_mutex.
 */
class Component : public Part, public std::enable_shared_from_this<Component> {
  public:
    /**
     * @brief Constructs a Component with the given name.
     * @param name A human-readable name for this Component.
     */
    explicit Component(const std::string& name);
    virtual ~Component();

    /** @brief Returns the name of this Component. */
    const std::string& GetName() const;

    /** @brief Returns a human-readable string representation (e.g. "Name (id)"). */
    std::string ToString() const;

    /** @brief Conversion operator to std::string; equivalent to ToString(). */
    explicit operator std::string() const;

#ifdef COMPONENT_THREAD_SAFE
    /**
     * @brief Returns a reference to the Component's shared mutex.
     *
     * Only available when COMPONENT_THREAD_SAFE is defined. Callers can use
     * this to synchronize external access to the Component.
     */
    std::shared_mutex& GetMutex() const;
#endif

    // ---- Parent/child relationship accessors ----

    /** @brief Returns a mutable reference to the parent list. */
    PartList<Component>& GetParents();
    /** @brief Returns a const reference to the parent list. */
    const PartList<Component>& GetParents() const;

    /** @brief Returns a mutable reference to the child list. */
    PartList<Component>& GetChildren();
    /** @brief Returns a const reference to the child list. */
    const PartList<Component>& GetChildren() const;

  private:
    std::string mName;
    PartList<Component> mParents;
    PartList<Component> mChildren;
#ifdef COMPONENT_THREAD_SAFE
    mutable std::shared_mutex mMutex;
#endif
};

} // namespace Ship
