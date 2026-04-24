#pragma once

#include <atomic>
#include <stdint.h>

namespace Ship {

/** @brief Sentinel value representing an invalid or unassigned Part ID. */
#define INVALID_PART_ID UINT64_MAX

/**
 * @brief Base class for all identifiable objects in the component system.
 *
 * Every Part is automatically assigned a unique, monotonically increasing ID
 * at construction time. IDs are never reused during the lifetime of the process.
 */
class Part {
  public:
    /** @brief Constructs a Part and assigns it a unique ID. */
    Part();
    virtual ~Part() = default;

    /** @brief Returns the unique identifier for this Part. */
    uint64_t GetId() const;

    /**
     * @brief Compares two Parts for equality by their unique IDs.
     * @param other The Part to compare against.
     * @return True if both Parts share the same ID.
     */
    bool operator==(const Part& other) const;

  private:
    static std::atomic<uint64_t> sNextPartId;
    uint64_t mId;
};

} // namespace Ship
