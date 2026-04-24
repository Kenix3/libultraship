#pragma once
#include <vector>

/**
 * @brief Callback function type invoked when a file is dropped onto the application window.
 * @param path Null-terminated path to the dropped file.
 * @return true if the handler consumed the event, false otherwise.
 */
typedef bool (*FileDroppedFunc)(char*);

namespace Ship {

/**
 * @brief Manages drag-and-drop file events for the application window.
 *
 * FileDropMgr tracks whether a file has been dropped, stores the dropped file
 * path, and dispatches the event to registered handler callbacks.
 */
class FileDropMgr {
  public:
    /** @brief Constructs a FileDropMgr with no registered handlers. */
    FileDropMgr() = default;

    /** @brief Destructor. */
    ~FileDropMgr();

    /**
     * @brief Records a file path as the currently dropped file.
     * @param path Null-terminated path to the dropped file.
     */
    void SetDroppedFile(char* path);

    /** @brief Clears the currently stored dropped file path and resets the drop state. */
    void ClearDroppedFile();

    /**
     * @brief Checks whether a file has been dropped since the last clear.
     * @return true if a file drop event is pending.
     */
    bool FileDropped() const;

    /**
     * @brief Returns the path of the most recently dropped file.
     * @return Null-terminated path string, or nullptr if no file has been dropped.
     */
    char* GetDroppedFile() const;

    /**
     * @brief Registers a callback to be invoked when a file is dropped.
     * @param func The callback function to register.
     * @return true if the handler was successfully registered.
     */
    bool RegisterDropHandler(FileDroppedFunc func);

    /**
     * @brief Unregisters a previously registered drop handler callback.
     * @param func The callback function to remove.
     * @return true if the handler was found and removed.
     */
    bool UnregisterDropHandler(FileDroppedFunc func);

    /** @brief Invokes all registered drop handler callbacks with the current dropped file path. */
    void CallHandlers();

  private:
    std::vector<FileDroppedFunc> mRegisteredFuncs;
    char* mPath = nullptr;
    bool mFileDropped = false;
};

} // namespace Ship
