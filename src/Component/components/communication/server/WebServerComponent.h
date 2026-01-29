#pragma once

#define MAX_CONCURRENT_UPLOADS 2

static AsyncWebServer server = AsyncWebServer(80);
static AsyncWebSocket ws("/");


DeclareComponentSingleton(WebServer, "server", )

    class WSPrint : public Print
{
public:
    uint8_t data[128]; // max 128 chars, should be enough
    int index = 0;
    size_t write(uint8_t c) override
    {
        if (index < 128)
            data[index++] = c;
        return 1;
    }
    void flush() { index = 0; };
};

DeclareBoolParam(sendFeedback, true);
DeclareBoolParam(sendDebugLogs, false);
DeclareBoolParam(suspendUpdatesDuringUpload, true);
DeclareBoolParam(suppressFeedbackDuringUpload, true);

WSPrint wsPrint;

bool wsIsInit = false;

bool isUploading;
int uploadedBytes;
File uploadingFile;
long timeAtLastCleanup = 0;

int activeUploadCount = 0;

std::string tmpExcludeParam = ""; // to change with client exclude when AsyncWebServer implements it

struct UploadFileState
{
    AsyncWebServerRequest *request = nullptr; // Use nullptr to check if the slot is free
    File file;
    bool active = false;
};

UploadFileState uploadingFiles[MAX_CONCURRENT_UPLOADS];

void setupInternal(JsonObject o) override;
bool initInternal() override;
void updateInternal() override;
void clearInternal() override;

void onEnabledChanged() override;

void setupConnection();
void closeServer();

void handleFileUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
void finishUploadForRequest(AsyncWebServerRequest *request, bool canceled);

#ifdef USE_OTA
void handleOTAUpload(AsyncWebServerRequest *request, size_t index, uint8_t *data, size_t len, bool final);
#endif

void onAsyncWSEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);

void parseTextMessage(std::string msg);
void parseBinaryMessage(uint8_t *data, size_t len);

void sendParamFeedback(Component *c, std::string pName, var *data, int numData);
void sendParamFeedback(std::string path, std::string pName, var *data, int numData);
void sendDebugLog(const std::string &msg, std::string source = "", std::string type = "info");
void sendBye(std::string type);

DeclareComponentEventTypes(UploadStart, Uploading, UploadDone, UploadCanceled);
DeclareComponentEventNames("UploadStart", "Uploading", "UploadDone", "UploadCanceled");


EndDeclareComponent