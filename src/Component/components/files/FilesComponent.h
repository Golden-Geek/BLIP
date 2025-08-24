#pragma once

// Core Arduino FS and LittleFS includes
#include <FS.h>
#include "LittleFS.h"

// Conditional includes based on the filesystem type
#ifdef FILES_TYPE_SD
#include "SD.h"
#endif
#ifdef FILES_TYPE_MMC
#include "SD_MMC.h"
#endif

// For external SPI Flash, we use ESP-IDF's VFS layer
#ifdef FILES_TYPE_FLASH
#include "esp_vfs.h"
#include "esp_vfs_fat.h"
#include "diskio_impl.h"
#include "diskio_rawflash.h"
#include "vfs_fat_internal.h" // Needed to get the fs::FS pointer
#endif

#ifndef FILES_DEFAULT_EN
#define FILES_DEFAULT_EN -1
#endif

// Forward declaration of the component
DeclareComponentSingleton(Files, "files", )

    //================================================================================
    // Public Methods
    //================================================================================
    void setupInternal(JsonObject o) override;
    bool initInternal() override;

    File openFile(String fileName, bool forWriting = false, bool deleteIfExists = true);
    bool deleteFolder(String path);
    void deleteFileIfExists(String path);
    String listDir(const char *dirname, uint8_t levels);
    String getFileSystemInfo();

    bool handleCommandInternal(const String &command, var *data, int numData) override;

    fs::FS& getFS() const { return *_fs; }
private:
    //================================================================================
    // Private Members
    //================================================================================

    // The single pointer to the active filesystem. This is the core of the refactor.
    fs::FS* _fs = nullptr;

    #if defined(FILES_TYPE_SD) || defined(FILES_TYPE_MMC)
    SPIClass spiSD;
    #elif defined FILES_TYPE_FLASH
    #endif

    #ifdef FILES_TYPE_FLASH
    // Handle for the wear levelling driver for the external flash
    wl_handle_t _wl_handle = WL_INVALID_HANDLE;
    #endif


    //================================================================================
    // Component Parameters
    //================================================================================
    #if defined(FILES_TYPE_SD) || defined(FILES_TYPE_FLASH)
        DeclareIntParam(sdSCK, FILES_DEFAULT_SCK);
        DeclareIntParam(sdMiso, FILES_DEFAULT_MISO);
        DeclareIntParam(sdMosi, FILES_DEFAULT_MOSI);
        DeclareIntParam(sdCS, FILES_DEFAULT_CS);
    #endif

    #ifdef FILES_TYPE_SD
        DeclareIntParam(sdSpeed, FILES_DEFAULT_SPEED);
        DeclareIntParam(sdEnPin, FILES_DEFAULT_EN);
        DeclareBoolParam(sdEnVal, FILES_DEFAULT_POWER_VALUE);
    #endif


    //================================================================================
    // Component Events and Settings
    //================================================================================
    DeclareComponentEventTypes(UploadStart, UploadProgress, UploadComplete, UploadCancel, FileList, FilesInfo);
    DeclareComponentEventNames("uploadStart", "uploadProgress", "uploadComplete", "uploadCancel", "list", "info");

    #ifdef FILES_TYPE_SD
        // Macros for SD card parameter handling...
        HandleSetParamInternalStart
            CheckAndSetParam(sdEnPin); CheckAndSetParam(sdEnVal); CheckAndSetParam(sdSCK);
            CheckAndSetParam(sdMiso); CheckAndSetParam(sdMosi); CheckAndSetParam(sdCS);
            CheckAndSetParam(sdSpeed);
        HandleSetParamInternalEnd;

        FillSettingsInternalStart
            FillSettingsParam(sdEnPin); FillSettingsParam(sdEnVal); FillSettingsParam(sdSCK);
            FillSettingsParam(sdMiso); FillSettingsParam(sdMosi); FillSettingsParam(sdCS);
            FillSettingsParam(sdSpeed);
        FillSettingsInternalEnd;

        FillOSCQueryInternalStart
            FillOSCQueryIntParam(sdEnPin); FillOSCQueryBoolParam(sdEnVal); FillOSCQueryIntParam(sdSCK);
            FillOSCQueryIntParam(sdMiso); FillOSCQueryIntParam(sdMosi); FillOSCQueryIntParam(sdCS);
            FillOSCQueryIntParam(sdSpeed);
        FillOSCQueryInternalEnd
    #endif

EndDeclareComponent
