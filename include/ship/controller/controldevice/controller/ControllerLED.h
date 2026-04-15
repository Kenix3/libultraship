#pragma once

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <string>

#include "ship/controller/controldevice/controller/mapping/ControllerLEDMapping.h"

namespace Ship {
/**
 * @brief Aggregates LED output mappings and drives controller lighting hardware.
 *
 * ControllerLED holds a collection of ControllerLEDMapping instances and fans out
 * SetLEDColor() calls to each one so that a single logical colour change activates
 * the LED on every mapped physical device simultaneously.
 *
 * Mappings are persisted in Config and applied via the standard load/save helpers.
 */
class ControllerLED {
  public:
    /**
     * @brief Constructs a ControllerLED for the given port.
     * @param portIndex Zero-based port index.
     */
    ControllerLED(uint8_t portIndex);
    ~ControllerLED();

    /**
     * @brief Returns a copy of the full id → mapping map.
     */
    std::unordered_map<std::string, std::shared_ptr<ControllerLEDMapping>> GetAllLEDMappings();

    /**
     * @brief Adds an LED mapping to the collection.
     * @param mapping Mapping to add. Its ID must be unique.
     */
    void AddLEDMapping(std::shared_ptr<ControllerLEDMapping> mapping);

    /**
     * @brief Removes the mapping ID from Config without destroying the in-memory object.
     * @param id Mapping UUID.
     */
    void ClearLEDMappingId(std::string id);

    /**
     * @brief Removes the mapping with @p id from both in-memory and Config.
     * @param id Mapping UUID.
     */
    void ClearLEDMapping(std::string id);

    /** @brief Writes the current set of mapping IDs to Config. */
    void SaveLEDMappingIdsToConfig();

    /** @brief Removes all in-memory LED mappings. */
    void ClearAllMappings();

    /**
     * @brief Removes all mappings that target the given device type.
     * @param physicalDeviceType Device type whose mappings should be cleared.
     */
    void ClearAllMappingsForDeviceType(PhysicalDeviceType physicalDeviceType);

    /**
     * @brief Loads the mapping with @p id from Config and adds it to the collection.
     * @param id Mapping UUID stored in Config.
     */
    void LoadLEDMappingFromConfig(std::string id);

    /** @brief Clears all in-memory mappings and reloads them from Config. */
    void ReloadAllMappingsFromConfig();

    /**
     * @brief Listens for the next raw physical input and creates an LED mapping from it.
     * @return true if a mapping was successfully created.
     */
    bool AddLEDMappingFromRawPress();

    /**
     * @brief Sends @p color to all mapped physical devices.
     * @param color RGB colour to apply to the controller LED(s).
     */
    void SetLEDColor(Color_RGB8 color);

    /**
     * @brief Returns true if any mapping targets the given device type.
     * @param physicalDeviceType Device type to query.
     */
    bool HasMappingsForPhysicalDeviceType(PhysicalDeviceType physicalDeviceType);

  private:
    uint8_t mPortIndex;
    std::unordered_map<std::string, std::shared_ptr<ControllerLEDMapping>> mLEDMappings;
};
} // namespace Ship
