#pragma once

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <string>

#include "ship/controller/controldevice/controller/mapping/ControllerRumbleMapping.h"

namespace Ship {
/**
 * @brief Aggregates rumble output mappings and drives force-feedback hardware.
 *
 * ControllerRumble holds a collection of ControllerRumbleMapping instances (one per
 * physical device output) and fans out StartRumble() / StopRumble() calls to all
 * of them. Multiple mappings allow the same logical rumble request to activate
 * haptics on several devices simultaneously.
 *
 * Mappings are persisted in Config and applied via the standard load/save helpers.
 */
class ControllerRumble {
  public:
    /**
     * @brief Constructs a ControllerRumble for the given port.
     * @param portIndex Zero-based port index.
     */
    ControllerRumble(uint8_t portIndex);
    ~ControllerRumble();

    /**
     * @brief Returns a copy of the full id → mapping map.
     */
    std::unordered_map<std::string, std::shared_ptr<ControllerRumbleMapping>> GetAllRumbleMappings();

    /**
     * @brief Adds a rumble mapping to the collection.
     * @param mapping Mapping to add. Its ID must be unique.
     */
    void AddRumbleMapping(std::shared_ptr<ControllerRumbleMapping> mapping);

    /**
     * @brief Removes the mapping ID from Config without destroying the in-memory object.
     * @param id Mapping UUID.
     */
    void ClearRumbleMappingId(std::string id);

    /**
     * @brief Removes the mapping with @p id from both in-memory and Config.
     * @param id Mapping UUID.
     */
    void ClearRumbleMapping(std::string id);

    /** @brief Writes the current set of mapping IDs to Config. */
    void SaveRumbleMappingIdsToConfig();

    /** @brief Removes all in-memory rumble mappings. */
    void ClearAllMappings();

    /**
     * @brief Removes all mappings that target the given device type.
     * @param physicalDeviceType Device type whose mappings should be cleared.
     */
    void ClearAllMappingsForDeviceType(PhysicalDeviceType physicalDeviceType);

    /**
     * @brief Applies the default rumble mappings for the given device type.
     * @param physicalDeviceType Device type to apply defaults for.
     */
    void AddDefaultMappings(PhysicalDeviceType physicalDeviceType);

    /**
     * @brief Loads the mapping with @p id from Config and adds it to the collection.
     * @param id Mapping UUID stored in Config.
     */
    void LoadRumbleMappingFromConfig(std::string id);

    /** @brief Clears all in-memory mappings and reloads them from Config. */
    void ReloadAllMappingsFromConfig();

    /**
     * @brief Listens for the next raw physical input and creates a rumble mapping from it.
     * @return true if a mapping was successfully created.
     */
    bool AddRumbleMappingFromRawPress();

    /** @brief Activates rumble on all mapped physical devices. */
    void StartRumble();

    /** @brief Stops rumble on all mapped physical devices. */
    void StopRumble();

    /**
     * @brief Returns true if any mapping targets the given device type.
     * @param physicalDeviceType Device type to query.
     */
    bool HasMappingsForPhysicalDeviceType(PhysicalDeviceType physicalDeviceType);

  private:
    uint8_t mPortIndex;
    std::unordered_map<std::string, std::shared_ptr<ControllerRumbleMapping>> mRumbleMappings;
};
} // namespace Ship
