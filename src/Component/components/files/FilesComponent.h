#pragma once



DeclareComponentSingleton(Files, "files", )

    void setupInternal(JsonObject o) override;
bool initInternal() override;
bool initInternalMemory();

#ifdef FILES_TYPE_LittleFS
#define FS_TYPE LittleFS
#elif defined FILES_TYPE_MMC
#define FS_TYPE SD_MMC
#else
#define FS_TYPE SD
SPIClass spiSD;
#endif

static fs::FS &fs;

bool useInternalMemory;
#ifdef FILES_TYPE_SD
DeclareIntParam(sdEnPin, FILES_DEFAULT_EN);
DeclareBoolParam(sdEnVal, FILES_DEFAULT_POWER_VALUE);
DeclareIntParam(sdSCK, FILES_DEFAULT_SCK);
DeclareIntParam(sdMiso, FILES_DEFAULT_MISO);
DeclareIntParam(sdMosi, FILES_DEFAULT_MOSI);
DeclareIntParam(sdCS, FILES_DEFAULT_CS);
DeclareIntParam(sdSpeed, FILES_DEFAULT_SPEED);
#endif

File openFile(String fileName, bool forWriting = false, bool deleteIfExists = true);
bool deleteFolder(String path);
void deleteFileIfExists(String path);
String listDir(const char *dirname, uint8_t levels);

bool handleCommandInternal(const String &command, var *data, int numData) override;
esp_err_t format_sdcard();

DeclareComponentEventTypes(UploadStart, UploadProgress, UploadComplete, UploadCancel, FileList);
DeclareComponentEventNames("uploadStart", "uploadProgress", "uploadComplete", "uploadCancel", "list");

#ifdef FILES_TYPE_SD
HandleSetParamInternalStart
    CheckAndSetParam(sdEnPin);
CheckAndSetParam(sdEnVal);
CheckAndSetParam(sdSCK);
CheckAndSetParam(sdMiso);
CheckAndSetParam(sdMosi);
CheckAndSetParam(sdCS);
CheckAndSetParam(sdSpeed);
HandleSetParamInternalEnd;

FillSettingsInternalStart
    FillSettingsParam(sdEnPin);
FillSettingsParam(sdEnVal);
FillSettingsParam(sdSCK);
FillSettingsParam(sdMiso);
FillSettingsParam(sdMosi);
FillSettingsParam(sdCS);
FillSettingsParam(sdSpeed);
FillSettingsInternalEnd;

FillOSCQueryInternalStart
    FillOSCQueryIntParam(sdEnPin);
FillOSCQueryBoolParam(sdEnVal);
FillOSCQueryIntParam(sdSCK);
FillOSCQueryIntParam(sdMiso);
FillOSCQueryIntParam(sdMosi);
FillOSCQueryIntParam(sdCS);
FillOSCQueryIntParam(sdSpeed);
FillOSCQueryInternalEnd
#endif

    EndDeclareComponent
