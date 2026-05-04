#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>
#include "ship/Component.h"

namespace Ship {

/**
 * @brief Describes where a key originated.
 *
 * Used to categorize keys so that system-managed and user-supplied keys
 * can be distinguished from keys bundled with game archives.
 */
enum class KeyOrigin { User, Game, System };

/**
 * @brief A single named key stored in the Keystore.
 */
struct KeystoreEntry {
    /** @brief Human-readable name identifying this key. */
    std::string Name;
    /** @brief Raw key bytes. */
    std::vector<uint8_t> Data;
    /** @brief Origin category for this key. */
    KeyOrigin Origin;
};

/**
 * @brief Persistent store of cryptographic keys used for archive signature verification.
 *
 * Keystore manages a collection of named binary keys that the engine uses to verify
 * the authenticity of resource archives. Keys can originate from the game distribution,
 * the user, or the engine itself. The store can be serialized to and from disk via
 * Load() and Save().
 *
 * **Required Context children (looked up at runtime):**
 * - **Config** — used by Load() and Save() to determine the path of the key
 *   persistence file. Config must be added to the Context before Keystore::Load()
 *   or Keystore::Save() are called.
 *
 * Obtain the instance from `Context::GetChildren().GetFirst<Keystore>()`.
 */
class Keystore : public Component {
  public:
    /**
     * @brief Constructs an empty Keystore.
     */
    Keystore();
    ~Keystore() override = default;

    /**
     * @brief Loads all keys from the persistent storage file on disk.
     */
    void Load();

    /**
     * @brief Persists the current set of keys to the storage file on disk.
     */
    void Save();

    /**
     * @brief Adds a key to the store.
     * @param keyName Name to associate with the key.
     * @param keyData Raw key bytes.
     * @param origin  Origin category for the key; defaults to KeyOrigin::Game.
     * @return true if the key was added, false if a key with that name already exists.
     */
    bool AddKey(const std::string& keyName, const std::vector<uint8_t>& keyData, KeyOrigin origin = KeyOrigin::Game);

    /**
     * @brief Removes a key from the store by name.
     * @param keyName Name of the key to remove.
     * @return true if the key was found and removed, false otherwise.
     */
    bool RemoveKey(const std::string& keyName);

    /**
     * @brief Checks whether a key with the given data exists in the store.
     * @param keyData Raw key bytes to search for.
     * @return true if a matching key is present.
     */
    bool HasKey(const std::vector<uint8_t>& keyData) const;

    /**
     * @brief Retrieves all keys that match the given name.
     * @param keyName Name to look up.
     * @return Vector of matching KeystoreEntry objects (may be empty).
     */
    std::vector<KeystoreEntry> GetKey(const std::string& keyName) const;

    /**
     * @brief Returns every key currently stored.
     * @return Vector of all KeystoreEntry objects.
     */
    std::vector<KeystoreEntry> GetAllKeys() const;

  private:
    std::unordered_map<std::string, KeystoreEntry> mKeys;
};

} // namespace Ship