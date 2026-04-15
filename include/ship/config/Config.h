#pragma once

#include <vector>
#include <string>
#include <nlohmann/json.hpp>

#include "ship/audio/Audio.h"
#include "ship/window/Window.h"

namespace Ship {

/**
 * @brief Abstract class representing a Config Version Updater, intended to express how to
 * upgrade a Configuration file from one version of a config to another (i.e. removing
 * default values, changing option names, etc.) It can be used by subclassing `ConfigVersionUpdater`,
 * implementing the Update function, and implementing the Constructor passing the version that the
 * Config is being updated to to this class' constructor from the child class' default constructor.
 * For example: \code ConfigVersion1Updater() : ConfigVersionUpdater(1) {} \endcode
 * Finally, give an instance of this subclass to a Config object via
 * RegisterConfigVersionUpdater and call RunVersionUpdates.
 */
class ConfigVersionUpdater {
  protected:
    uint32_t mVersion;

  public:
    ConfigVersionUpdater(uint32_t toVersion);
    /**
     * @brief Performs actions on a Config object via the provided pointer to update it
     * to the next version. (i.e. removing/changing default values or renaming options)
     *
     * @param conf
     */
    virtual void Update(Config* conf) = 0;

    /**
     * @brief Get the value of mVersion
     *
     * @return uint32_t
     */
    uint32_t GetVersion();
};

/**
 * @brief Persistent JSON-backed key-value configuration store.
 *
 * Config provides typed accessors (Get/Set) for settings stored in a flat JSON file.
 * Keys may use dot-notation to express nesting (e.g. "Window.Width"). The file is read
 * on construction and can be reloaded or saved at any time.
 *
 * Version migration is supported through ConfigVersionUpdater subclasses; register them
 * with RegisterVersionUpdater() and call RunVersionUpdates() on startup.
 *
 * Obtain the instance from Context::GetConfig().
 */
class Config {
  public:
    /**
     * @brief Constructs a Config, loading the JSON file at @p path (creates it if absent).
     * @param path Filesystem path to the JSON configuration file.
     */
    Config(std::string path);
    ~Config();

    /**
     * @brief Returns the string value for @p key, or @p defaultValue if absent.
     * @param key          Dot-notation config key.
     * @param defaultValue Fallback string.
     */
    std::string GetString(const std::string& key, const std::string& defaultValue = "");

    /**
     * @brief Returns the float value for @p key, or @p defaultValue if absent.
     * @param key          Dot-notation config key.
     * @param defaultValue Fallback value.
     */
    float GetFloat(const std::string& key, float defaultValue = 0.0f);

    /**
     * @brief Returns the boolean value for @p key, or @p defaultValue if absent.
     * @param key          Dot-notation config key.
     * @param defaultValue Fallback value.
     */
    bool GetBool(const std::string& key, bool defaultValue = false);

    /**
     * @brief Returns the signed integer value for @p key, or @p defaultValue if absent.
     * @param key          Dot-notation config key.
     * @param defaultValue Fallback value.
     */
    int32_t GetInt(const std::string& key, int32_t defaultValue = 0);

    /**
     * @brief Returns the unsigned integer value for @p key, or @p defaultValue if absent.
     * @param key          Dot-notation config key.
     * @param defaultValue Fallback value.
     */
    uint32_t GetUInt(const std::string& key, uint32_t defaultValue = 0);

    /**
     * @brief Writes a string value for @p key (creates the key if it does not exist).
     * @param key   Dot-notation config key.
     * @param value Value to store.
     */
    void SetString(const std::string& key, const std::string& value);

    /**
     * @brief Writes a float value for @p key.
     * @param key   Dot-notation config key.
     * @param value Value to store.
     */
    void SetFloat(const std::string& key, float value);

    /**
     * @brief Writes a boolean value for @p key.
     * @param key   Dot-notation config key.
     * @param value Value to store.
     */
    void SetBool(const std::string& key, bool value);

    /**
     * @brief Writes a signed integer value for @p key.
     * @param key   Dot-notation config key.
     * @param value Value to store.
     */
    void SetInt(const std::string& key, int32_t value);

    /**
     * @brief Writes an unsigned integer value for @p key.
     * @param key   Dot-notation config key.
     * @param value Value to store.
     */
    void SetUInt(const std::string& key, uint32_t value);

    /**
     * @brief Removes a single key from the config.
     * @param key Dot-notation config key to remove.
     */
    void Erase(const std::string& key);

    /**
     * @brief Removes all keys that share the given prefix (dot-notation block).
     * @param key Prefix of the block to erase (e.g. "Window" removes all "Window.*" keys).
     */
    void EraseBlock(const std::string& key);

    /**
     * @brief Replaces the JSON object stored at @p key with @p block.
     * @param key   Dot-notation key pointing to a JSON object node.
     * @param block Replacement JSON object.
     */
    void SetBlock(const std::string& key, nlohmann::json block);

    /**
     * @brief Copies the value at @p fromKey to @p toKey (overwriting any existing value at @p toKey).
     * @param fromKey Source dot-notation key.
     * @param toKey   Destination dot-notation key.
     */
    void Copy(const std::string& fromKey, const std::string& toKey);

    /**
     * @brief Returns true if @p key exists in the config.
     * @param key Dot-notation config key to check.
     */
    bool Contains(const std::string& key);

    /** @brief Discards in-memory values and reloads the config file from disk. */
    void Reload();

    /** @brief Flushes in-memory values to the config file on disk. */
    void Save();

    /**
     * @brief Returns the full config as a nested JSON object.
     *
     * Unlike the internal flattened representation, the returned object mirrors the
     * dot-notation key hierarchy as nested JSON objects.
     */
    nlohmann::json GetNestedJson();

    /** @brief Returns the audio backend identifier stored in the config. */
    AudioBackend GetCurrentAudioBackend();

    /**
     * @brief Persists the given audio backend identifier to the config.
     * @param backend Backend to store.
     */
    void SetCurrentAudioBackend(AudioBackend backend);

    /** @brief Returns the window backend identifier stored in the config. */
    WindowBackend GetWindowBackend();

    /**
     * @brief Persists the given window backend identifier to the config.
     * @param backend Backend to store.
     */
    void SetWindowBackend(WindowBackend backend);

    /** @brief Returns the audio channel layout stored in the config. */
    AudioChannelsSetting GetCurrentAudioChannelsSetting();

    /**
     * @brief Adds a ConfigVersionUpdater instance to the list to be run later via RunVersionUpdates
     *
     * @param versionUpdater
     * @return true if the insert was successful, or
     * @return false if the insert failed, i.e. if the list already has a ConfigVersionUpdater with
     * a matching version.
     */
    bool RegisterVersionUpdater(std::shared_ptr<ConfigVersionUpdater> versionUpdater);

    /**
     * @brief Runs the Update function on each ConfigVersionUpdater instance if the version matches\
     * the current ConfigVersion value of the config object.
     *
     */
    void RunVersionUpdates();

  protected:
    /**
     * @brief Resolves a dot-notation key into its nested JSON node.
     * @param key Dot-notation config key.
     * @return The JSON value at that node.
     */
    nlohmann::json Nested(const std::string& key);

    /**
     * @brief Converts a dot-notation key into the slash-separated flat key format used internally.
     * @param key Dot-notation key.
     * @return Formatted flat key string.
     */
    static std::string FormatNestedKey(const std::string& key);

    /**
     * @brief Writes an array value for @p key.
     * @tparam T  Element type (must be JSON-serializable).
     * @param key   Dot-notation config key.
     * @param array Values to store.
     */
    template <typename T> void SetArray(const std::string& key, std::vector<T> array);

    /**
     * @brief Reads an array value for @p key.
     * @tparam T  Element type (must be JSON-deserializable).
     * @param key Dot-notation config key.
     * @return Vector of values, or an empty vector if the key is absent.
     */
    template <typename T> std::vector<T> GetArray(const std::string& key);

  private:
    nlohmann::json mFlattenedJson;
    nlohmann::json mNestedJson;
    std::string mPath;
    bool mIsNewInstance;
    std::map<uint32_t, std::shared_ptr<ConfigVersionUpdater>> mVersionUpdaters;
};
} // namespace Ship
