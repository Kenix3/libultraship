//
//  AppleFolderManager.h
//  libultraship
//
//  Created by David Chavez on 28.06.22.
//

#pragma once

#include <stdio.h>
namespace Ship {

/**
 * @brief macOS / iOS standard-directory constants mirroring NSSearchPathDirectory.
 *
 * These values match the macOS SDK's NSSearchPathDirectory enumeration so that the
 * Ship code can locate standard directories (Documents, Caches, Application Support,
 * etc.) without introducing an Objective-C dependency in C++ translation units.
 */
enum {
    NSApplicationDirectory = 1,       ///< Applications directory (/Applications).
    NSDemoApplicationDirectory,       ///< Demonstration applications directory.
    NSDeveloperApplicationDirectory,  ///< Developer applications directory.
    NSAdminApplicationDirectory,      ///< Administration applications directory.
    NSLibraryDirectory,               ///< Library directory.
    NSDeveloperDirectory,             ///< Developer tools directory.
    NSUserDirectory,                  ///< User home directories (~/...).
    NSDocumentationDirectory,         ///< Documentation directory.
    NSDocumentDirectory,              ///< Documents directory (~/Documents).
    NSCoreServiceDirectory,           ///< Core services directory.
    NSAutosavedInformationDirectory = 11, ///< Autosaved information directory.
    NSDesktopDirectory = 12,          ///< Desktop directory (~/Desktop).
    NSCachesDirectory = 13,           ///< Caches directory (~/Library/Caches).
    NSApplicationSupportDirectory = 14, ///< Application support directory (~/Library/Application Support).
    NSDownloadsDirectory = 15,        ///< Downloads directory (~/Downloads).
    NSInputMethodsDirectory = 16,     ///< Input methods directory.
    NSMoviesDirectory = 17,           ///< Movies directory (~/Movies).
    NSMusicDirectory = 18,            ///< Music directory (~/Music).
    NSPicturesDirectory = 19,         ///< Pictures directory (~/Pictures).
    NSPrinterDescriptionDirectory = 20, ///< Printer description directory.
    NSSharedPublicDirectory = 21,     ///< Public directory (~/Public).
    NSPreferencePanesDirectory = 22,  ///< Preference panes directory.
    NSApplicationScriptsDirectory = 23, ///< Application scripts directory.
    NSItemReplacementDirectory = 99,  ///< Temporary directory suitable for atomic file replacement.
    NSAllApplicationsDirectory = 100, ///< All applications directories.
    NSAllLibrariesDirectory = 101,    ///< All library directories.
    NSTrashDirectory = 102            ///< Trash directory.
};

/** @brief Type alias for a search-path directory constant (see NSSearchPathDirectory values above). */
typedef unsigned long SearchPathDirectory;

/**
 * @brief macOS / iOS domain-mask constants mirroring NSSearchPathDomainMask.
 *
 * Domain masks are used together with SearchPathDirectory values to restrict which
 * domain(s) FolderManager searches when locating a standard directory.
 */
enum {
    NSUserDomainMask = 1,       ///< User's home directory (~).
    NSLocalDomainMask = 2,      ///< Local machine directory (/Library).
    NSNetworkDomainMask = 4,    ///< Network directory (/Network).
    NSSystemDomainMask = 8,     ///< System directory (/System) — provided by Apple, unmodifiable.
    NSAllDomainsMask = 0x0ffff  ///< All domains.
};

/** @brief Type alias for a domain-mask value (see NSSearchPathDomainMask values above). */
typedef unsigned long SearchPathDomainMask;

/**
 * @brief Thin Objective-C++ bridge for locating macOS standard directories.
 *
 * FolderManager wraps a hidden NSAutoreleasePool and delegates to
 * NSSearchPathForDirectoriesInDomains() so that C++ code can locate standard
 * directories (Application Support, Caches, main bundle, etc.) without including
 * Objective-C headers directly.
 *
 * This class is only available on Apple platforms.
 */
class FolderManager {
  public:
    /** @brief Creates the internal NSAutoreleasePool. */
    FolderManager();
    ~FolderManager();

    /**
     * @brief Creates the Application Support sub-directory for @p appName if it does not already exist.
     * @param appName Application name used as the sub-directory name inside Application Support.
     */
    void CreateAppSupportDirectory(const char* appName);

    /**
     * @brief Returns the path to the application's main bundle directory.
     * @return Null-terminated UTF-8 path (valid for the lifetime of this FolderManager).
     */
    const char* getMainBundlePath();

    /**
     * @brief Returns the first path for the given directory and domain mask.
     * @param directory  Standard directory constant (e.g. NSApplicationSupportDirectory).
     * @param domainMask Domain(s) to search (e.g. NSUserDomainMask).
     * @return Null-terminated UTF-8 path, or nullptr on failure.
     */
    const char* pathForDirectory(SearchPathDirectory directory, SearchPathDomainMask domainMask);

    /**
     * @brief Returns a path appropriate for storing a file at @p itemPath.
     *
     * Optionally creates the returned directory if @p create is true.
     *
     * @param directory  Standard directory constant.
     * @param domainMask Domain(s) to search.
     * @param itemPath   Path of the item to be stored (used as a hint).
     * @param create     If true, the directory is created before the path is returned.
     * @return Null-terminated UTF-8 path, or nullptr on failure.
     */
    const char* pathForDirectoryAppropriateForItemAtPath(SearchPathDirectory directory, SearchPathDomainMask domainMask,
                                                         const char* itemPath, bool create = false);

  private:
    void* m_autoreleasePool; ///< Opaque pointer to the NSAutoreleasePool instance.
};
}; // namespace Ship
