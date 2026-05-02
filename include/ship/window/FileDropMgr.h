#pragma once
#include <vector>
#include <memory>
#include "ship/Component.h"
#include "ship/window/Window.h"

/**
 * @brief Callback type for file-drop event handlers.
 *
 * A handler receives the path of the dropped file and returns true if it
 * consumed the event, or false to let other handlers process it.
 */
typedef bool (*FileDroppedFunc)(char*);

namespace Ship {

/**
 * @brief Handles drag-and-drop file events for the application window.
 *
 * FileDropMgr stores the path of the most recently dropped file and dispatches
 * the event to a chain of registered handler callbacks.
 *
 * **Required Context children (looked up at Init time):**
 * - **Window** — cached in OnInit() and used by the drop-event dispatcher to obtain the GUI
 *   layer when forwarding drop events. Window must be added to the Context and initialized
 *   before FileDropMgr::Init() is called.
 *
 * Obtain the instance from `Context::GetChildren().GetFirst<FileDropMgr>()`.
 */
class FileDropMgr : public Component {
  public:
    /** @brief Constructs the FileDropMgr component. */
    FileDropMgr();
    ~FileDropMgr();

    /**
     * @brief Records a file path as having been dropped onto the window.
     * @param path Path of the dropped file (ownership is not taken).
     */
    void SetDroppedFile(char* path);

    /** @brief Clears the current dropped-file state. */
    void ClearDroppedFile();

    /** @brief Returns true if a file has been dropped and not yet cleared. */
    bool FileDropped() const;

    /**
     * @brief Returns the path of the most recently dropped file.
     * @return File path, or nullptr if no file has been dropped.
     */
    char* GetDroppedFile() const;

    /**
     * @brief Registers a callback to be invoked when a file is dropped.
     * @param func Handler function to register.
     * @return true if the handler was successfully added.
     */
    bool RegisterDropHandler(FileDroppedFunc func);

    /**
     * @brief Removes a previously registered drop handler.
     * @param func Handler function to unregister.
     * @return true if the handler was found and removed.
     */
    bool UnregisterDropHandler(FileDroppedFunc func);

    /** @brief Invokes all registered drop handlers with the current dropped file. */
    void CallHandlers();

  protected:
    /** @brief Caches the Window component from the Context hierarchy. */
    void OnInit() override;

  private:
    std::vector<FileDroppedFunc> mRegisteredFuncs;
    char* mPath = nullptr;
    bool mFileDropped = false;
    std::shared_ptr<Window> mWindow;

    /** @brief Returns the cached Window component. */
    std::shared_ptr<Window> GetWindow() const;
};

} // namespace Ship
