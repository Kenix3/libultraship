#pragma once

#include <atomic>
#include <memory>
#include <stdint.h>

namespace Ship {

class Context;

/** @brief Sentinel value representing an invalid or unassigned Part ID. */
#define INVALID_PART_ID UINT64_MAX

/**
 * @brief Base class for all identifiable objects in the component system.
 *
 * Every Part is automatically assigned a unique, monotonically increasing ID
 * at construction time. IDs are never reused during the lifetime of the process.
 *
 * Every Part also stores a weak reference to the Context it belongs to. This
 * reference is set when the Part is added to the component hierarchy and
 * propagated to child components by ComponentList.
 */
class Part {
  public:
    /** @brief Constructs a Part and assigns it a unique ID. */
    Part();
    /** @brief Constructs a Part with an explicit Context reference and unique ID. */
    explicit Part(std::shared_ptr<Context> context);
    virtual ~Part() = default;

    /** @brief Returns the unique identifier for this Part. */
    uint64_t GetId() const;

    /**
     * @brief Compares two Parts for equality by their unique IDs.
     * @param other The Part to compare against.
     * @return True if both Parts share the same ID.
     */
    bool operator==(const Part& other) const;

    /**
     * @brief Returns the Context this Part belongs to, or nullptr if unset.
     *
     * The context is propagated automatically by ComponentList when this Part
     * is added to a hierarchy. Components should cache their dependencies from
     * this Context (or from constructor parameters) rather than using the
     * Context singleton.
     */
    std::shared_ptr<Context> GetContext() const;

    /**
     * @brief Sets the Context this Part belongs to.
     *
     * Called by ComponentList when a Part is added to the hierarchy. Not
     * normally called directly by application code.
     *
     * @param ctx The Context to associate with this Part.
     */
    void SetContext(std::shared_ptr<Context> ctx);

  protected:
    /**
     * @brief Called after this Part has been added to a PartList.
     *
     * Override in subclasses to react to being added (e.g. caching siblings).
     * The default implementation is a no-op.
     *
     * @param forced Whether the addition bypassed permission checks.
     */
    virtual void OnAdded(bool forced);

    /**
     * @brief Called after this Part has been removed from a PartList.
     *
     * Override in subclasses to react to being removed (e.g. clearing caches).
     * The default implementation is a no-op.
     *
     * @param forced Whether the removal bypassed permission checks.
     */
    virtual void OnRemoved(bool forced);

    template <typename> friend class PartList;

  private:
    static std::atomic<uint64_t> sNextPartId;
    uint64_t mId;
    std::weak_ptr<Context> mContext;
};

} // namespace Ship
