#pragma once

#define MAX_CONCURRENT_UPLOADS 5

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

WSPrint wsPrint;

bool wsIsInit = false;

bool isUploading;
int uploadedBytes;
File uploadingFile;
long timeAtLastCleanup = 0;

String tmpExcludeParam = ""; // to change with client exclude when AsyncWebServer implements it

struct UploadFileState
{
    AsyncWebServerRequest *request = nullptr; // Use nullptr to check if the slot is free
    File file;
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

#ifdef USE_ASYNC_WEBSOCKET
void onAsyncWSEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);
#else
void onWSEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length);
#endif

// void sendParameterFeedback(Component *c, Parameter *p);
void sendParamFeedback(Component *c, String pName, var *data, int numData);
void sendBye(String type);



DeclareComponentEventTypes(UploadStart, Uploading, UploadDone, UploadCanceled);
DeclareComponentEventNames("UploadStart", "Uploading", "UploadDone", "UploadCanceled");

HandleSetParamInternalStart
    CheckAndSetParam(sendFeedback);
HandleSetParamInternalEnd;

FillSettingsInternalStart
    FillSettingsParam(sendFeedback);
FillSettingsInternalEnd;

FillOSCQueryInternalStart
    FillOSCQueryBoolParam(sendFeedback);
FillOSCQueryInternalEnd;

EndDeclareComponent