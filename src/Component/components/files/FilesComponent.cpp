#include "UnityIncludes.h"
#include "FilesComponent.h"

ImplementSingleton(FilesComponent);

void FilesComponent::setupInternal(JsonObject o)
{
// Conditionally add parameters based on build flags
#if defined(FILES_TYPE_SD) || defined(FILES_TYPE_FLASH)
    AddIntParamConfig(sdSCK);
    AddIntParamConfig(sdMiso);
    AddIntParamConfig(sdMosi);
    AddIntParamConfig(sdCS);
#endif
#ifdef FILES_TYPE_SD
    AddIntParamConfig(sdEnPin);
    AddBoolParamConfig(sdEnVal);
    AddIntParamConfig(sdSpeed);
#endif
}

bool FilesComponent::initInternal()
{
    bool mounted = false;

#if defined(FILES_TYPE_MMC)
    NDBG("Attempting to initialize SD MMC...");
    if (SD_MMC.begin("/", true))
    {
        _fs = &SD_MMC;
        mounted = true;
        NDBG("SD MMC mounted successfully.");
    }
#elif defined(FILES_TYPE_SD)
    NDBG("Attempting to initialize SD Card via SPI...");
    if (sdSCK > 0 && sdMiso > 0 && sdMosi > 0 && sdCS > 0)
    {
        if (sdEnPin > 0)
        {
            pinMode(sdEnPin, OUTPUT);
            digitalWrite(sdEnPin, sdEnVal);
            delay(10);
        }

        NDBG("Initializing SPI for SD Card on pins MOSI " + String(sdMosi) + ", MISO " + String(sdMiso) + ", SCK " + String(sdSCK) + ",  CS " + String(sdCS));

        spiSD.begin((int8_t)sdSCK, (int8_t)sdMiso, (int8_t)sdMosi, (int8_t)sdCS);
        if (SD.begin((uint8_t)sdCS, spiSD, sdSpeed))
        {
            _fs = &SD;
            mounted = true;
            NDBG("SD Card mounted successfully.");
        }
    }
#elif defined(FILES_TYPE_FLASH)

    NDBG("Initializing SPI for Flash on pins MOSI " + String(sdMosi) + ", MISO " + String(sdMiso) + ", SCK " + String(sdSCK) + ",  CS " + String(sdCS));

    pinMode(sdSCK, OUTPUT);
    digitalWrite(sdSCK, LOW);
    pinMode(sdMiso, INPUT_PULLUP);
    pinMode(sdMosi, INPUT_PULLUP);
    pinMode(sdCS, OUTPUT);
    digitalWrite(sdCS, HIGH);

    SPI.begin(sdSCK, sdMiso, sdMosi, sdCS);
    if (!SD.begin(sdCS, SPI, 1'000'000UL))
    {
        Serial.println("Flash init failed");
    }
    else
    {
        _fs = &SD;
        mounted = true;
        NDBG("SD Card mounted successfully.");
    }
#endif

    // Fallback to internal memory if no external storage was mounted
    if (!mounted)
    {
        NDBG("No external storage mounted. Using internal LittleFS.");
        if (LittleFS.begin(true))
        {
            _fs = &LittleFS;
            mounted = true;
            NDBG("Internal LittleFS initialized.");
        }
        else
        {
            NDBG("CRITICAL: Failed to initialize internal LittleFS.");
            return false;
        }
    }

    // Create default directories on the active filesystem
    if (mounted && _fs)
    {
        // _fs->mkdir("/scripts");
        // _fs->mkdir("/playback");
        // _fs->mkdir("/server");
    }

    return mounted;
}

// --- The rest of the file operations remain unchanged and beautifully simple ---

File FilesComponent::openFile(String fileName, bool forWriting, bool deleteIfExists)
{
    if (!_fs)
        return File();

    String fullPath = fileName;
#ifdef FILES_TYPE_FLASH
    if (_fs != &LittleFS)
    { // Only prepend if we're not on the fallback internal FS
        fullPath = (fileName.startsWith("/") ? "" : "/") + fileName;
    }
#endif

    if (!fullPath.startsWith("/"))
    {
        fullPath = "/" + fullPath;
    }

    if (forWriting && deleteIfExists)
    {
        deleteFileIfExists(fullPath);
    }

    return _fs->open(fullPath.c_str(), forWriting ? FILE_WRITE : FILE_READ);
}

bool FilesComponent::deleteFolder(String path)
{
    if (!_fs || !isInit)
        return false;

    if (!path.startsWith("/"))
    {
        path = "/" + path;
    }

    File dir = _fs->open(path.c_str());
    if (!dir || !dir.isDirectory())
    {
        NDBG("Failed to open directory or not a directory: " + path);
        return false;
    }
    
    File file;
    while (file = dir.openNextFile())
    {
        if (file.isDirectory())
        {
            deleteFolder(file.path());
        }
        else
        {
            //here, because file is open, we cannot delete while iterating, so close first
            String path = String(file.path());
            file.close();
            NDBG("Deleting file: " + path);
            _fs->remove(path.c_str());
        }
    }

    _fs->rmdir(path.c_str());
    NDBG("Deleted folder: " + path);

    return true;
}

void FilesComponent::deleteFileIfExists(String path)
{
    if (!_fs || !isInit)
        return;
    if (_fs->exists(path.c_str()))
    {
        _fs->remove(path.c_str());
    }
}


void FilesComponent::createFolderIfNotExists(String path)
{
    if (!_fs || !isInit)
        return;

    if (!path.startsWith("/"))
    {
        path = "/" + path;
    }

    if (!_fs->exists(path.c_str()))
    {
        _fs->mkdir(path.c_str());
        NDBG("Created folder: " + path);
    }
}

/**
 * @brief Lists the contents of a directory.
 * @param dirname The directory to list.
 * @param levels The number of sub-directory levels to recurse into.
 * @return A comma-separated string of file paths.
 */
String FilesComponent::listDir(const char *dirname, uint8_t levels)
{
    if (!_fs)
        return "";

    NDBG("Listing directory: " + String(dirname) + " with levels: " + String(levels));

    String result = "";
    File root = _fs->open(dirname);
    if (!root || !root.isDirectory())
    {
        NDBG("Failed to open directory or not a directory.");
        return result;
    }

    File file;
    while (file = root.openNextFile())
    {
        if (file.isDirectory())
        {
            NDBG("  DIR : " + String(file.name()));
            if (levels > 0)
            {
                result += listDir(file.path(), levels - 1);
            }
        }
        else
        {
            NDBG("  FILE: " + String(file.name()) + " SIZE: " + String(file.size()));
            result += String(file.path()) + ",";
        }
    }
    return result;
}

/**
 * @brief Retrieves information about the current filesystem.
 */
String FilesComponent::getFileSystemInfo()
{
    if (!_fs)
        return "FS not mounted";


#if defined FILES_TYPE_FLASH || defined FILES_TYPE_SD
    String fsType = "SDCard/Flash";
    int bytesUsed = SD.usedBytes();
    int bytesTotal = SD.totalBytes();
#elif defined FILES_TYPE_MMC
    String fsType = "SDMMC";
    int bytesUsed = SD_MMC.usedBytes();
    int bytesTotal = SD_MMC.totalBytes();
#else
    String fsType = "LittleFS";
    int bytesUsed = LittleFS.usedBytes();
    int bytesTotal = LittleFS.totalBytes();
#endif

    String info = "Filesystem Info:\n \
    Type: " + fsType + "\n \
    Used % : " + String((bytesUsed * 100) / bytesTotal) + "%\n \
    Used Size: " + String(bytesUsed / 1024) + " KB\n\
    Total Size: " + String(bytesTotal / 1024) + " KB";

    return info;
}

/**
 * @brief Handles incoming commands for this component.
 */
bool FilesComponent::handleCommandInternal(const String &command, var *data, int numData)
{
    if (checkCommand(command, "delete", numData, 1))
    {
        deleteFileIfExists(data[0].stringValue());
        return true;
    }
    else if (checkCommand(command, "deleteFolder", numData, 1))
    {
        deleteFolder(data[0].stringValue());
        return true;
    }
    else if (checkCommand(command, "list", numData, 0))
    {
        var filesData;
        filesData.type = 's';
        int level = (numData == 1) ? data[0].intValue() : 0;
        NDBG("List directories with levels : " + String(level));
        filesData = listDir("/", level);
        sendEvent(FileList, &filesData, 1);
        return true;
    }
    else if (checkCommand(command, "format", numData, 0))
    {
        NDBG("Formatting SD card.");
        deleteFolder("/");
        return true;
    }
    else if (checkCommand(command, "info", numData, 0))
    {
        var infoData;
        infoData.type = 's';
        infoData.s = getFileSystemInfo();
        sendEvent(FilesInfo, &infoData, 1);
        return true;
    }else if(checkCommand(command, "deleteAll", numData, 0))
    {
        NDBG("Deleting all files and folders in filesystem.");
        return true;
    }

    return false;
}