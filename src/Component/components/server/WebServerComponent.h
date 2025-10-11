#pragma once

#define MAX_CONCURRENT_UPLOADS 5

#ifdef USE_ASYNC_WEBSOCKET
static AsyncWebServer server = AsyncWebServer(80);
static AsyncWebSocket ws("/");

#define RequestHasArg(name) request->hasArg(name)
#define RequestGetArg(name) request->arg(name)
#define SendRequestResponse(code, type, content) request->send(code, type, content)

#else
static WebServer server = WebServer(80);
static WebSocketsServer ws = WebSocketsServer(81);
#define RequestHasArg(name) server.hasArg(name)
#define RequestGetArg(name) server.arg(name)
#define SendRequestResponse(code, type, content) server.send(code, type, content)
#endif

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

#ifndef USE_ASYNC_WEBSOCKET
RequestHandler handler;
#endif


struct UploadFileState
{
#ifdef USE_ASYNC_WEBSOCKET
    AsyncWebServerRequest *request = nullptr; // Use nullptr to check if the slot is free
#endif
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

#ifdef USE_ASYNC_WEBSOCKET
void handleFileUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
void onAsyncWSEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);
#else
void onWSEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length);
#endif

void parseTextMessage(String msg);
void parseBinaryMessage(uint8_t *data, size_t len);

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