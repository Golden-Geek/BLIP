#include "UnityIncludes.h"

ImplementSingleton(WebServerComponent);

void WebServerComponent::setupInternal(JsonObject o)
{
    for (int i = 0; i < MAX_CONCURRENT_UPLOADS; i++)
    {
        uploadingFiles[i].request = nullptr;
    }

    AddBoolParamConfig(sendFeedback);
}

bool WebServerComponent::initInternal()
{
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, PUT");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");

    ws.onEvent(std::bind(&WebServerComponent::onAsyncWSEvent, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
    server.addHandler(&ws);

    server.on("/", HTTP_GET, [&](AsyncWebServerRequest *request)
              {
        RootComponent::instance->timeAtLastSignal = millis();

        if (request->hasArg("HOST_INFO"))
            {
            DynamicJsonDocument doc(1000);
            JsonObject o = doc.to<JsonObject>();
            JsonObject eo = o.createNestedObject("EXTENSIONS");
            eo["ACCESS"] = true;
            eo["CLIPMODE"] = false;
            eo["CRITICAL"] = false;
            eo["RANGE"] = true;
            eo["TAGS"] = false;
            eo["TYPE"] = true;
            eo["UNIT"] = false;
            eo["VALUE"] = true;
            eo["LISTEN"] = true;
            eo["PATH_ADDED"] = true;
            eo["PATH_REMOVED"] = true;
            eo["PATH_RENAMED"] = true;
            eo["PATH_CHANGED"] = false;

            o["NAME"] = DeviceName;
            o["VERSION"] = BLIP_VERSION;
            o["OSC_PORT"] = OSC_LOCAL_PORT;
            o["OSC_TRANSPORT"] = "UDP";

            String jStr;
            serializeJson(doc, jStr);

            request->send(200, "application/json", jStr);
        }
        else
        {
            std::shared_ptr<bool> showConfig = std::make_shared<bool>(request->hasArg("config") ? request->arg("config") == "1" : true);
            std::shared_ptr<OSCQueryChunk> chunk = std::make_shared<OSCQueryChunk>(OSCQueryChunk(RootComponent::instance));

            AsyncWebServerResponse *response = request->beginChunkedResponse("application/json", [chunk, showConfig](uint8_t *buffer, size_t maxLen, size_t index)
                                                                             {
                                                                                 if (chunk->nextComponent == nullptr)
                                                                                     return 0;
                                                                                 chunk->nextComponent->fillChunkedOSCQueryData(chunk.get(), *showConfig);
                                                                                 sprintf((char *)buffer, chunk->data.c_str());
                                                                                 return (int)chunk->data.length(); });

            request->send(response);
        } });

#ifdef USE_FILES
    NDBG("Setting up local files to serve");
    server.on(
        "/uploadFile", HTTP_POST, [](AsyncWebServerRequest *request)
        { request->send(200); },
        std::bind(&WebServerComponent::handleFileUpload,
                  this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));

    server.onNotFound([](AsyncWebServerRequest *request)
                      {

                        
        if(!FilesComponent::instance->isInit) {
            request->send(500, "text/plain", "Filesystem not initialized");
            return;
        }
        File f = FilesComponent::instance->openFile(request->url(), false, false);
        if(!f || f.isDirectory()) {

                        //if doesnt exist, try to find in /server folder, and add html if no extension
            String path = request->url();
            if(!path.startsWith("/server/")) {
                path = String("/server") + (path.startsWith("/")?"":"/") + path;
                f = FilesComponent::instance->openFile(path, false, false);
                if(!f || f.isDirectory()) {
                    if(!path.endsWith(".html") && !path.endsWith(".htm") && !path.endsWith(".css") && !path.endsWith(".js")) {
                        path += ".html";
                        f = FilesComponent::instance->openFile(path, false, false);

                        if(!f || f.isDirectory()) {
                            request->send(404, "text/plain", "File not found : " + request->url()+" (also tried "+path+")");
                            return;
                        }
                    }
                }
            }
        }

        DBG("Serving file: " + String(f.name()) + " size: " + String(f.size()));

        AsyncWebServerResponse *response = request->beginChunkedResponse("text/html", [f](uint8_t *buffer, size_t maxLen, size_t index) mutable {

            size_t len = f.read(buffer, maxLen);
            if(len == 0) {
                f.close();
            }
            return len;
        });
        DBG("Serving file chunk");
        request->send(response); });

#endif

    return true; // end Init Internal
}

void WebServerComponent::updateInternal()
{
    if (wsIsInit)
    {
        if (timeAtLastCleanup + 10000 < millis())
        {
            timeAtLastCleanup = millis();
            ws.cleanupClients();
        }
    }
}

void WebServerComponent::clearInternal()
{
    closeServer();
}

// SERVER
void WebServerComponent::onEnabledChanged()
{
    setupConnection();
}

void WebServerComponent::setupConnection()
{
    bool shouldConnect = enabled && WifiComponent::instance->state == WifiComponent::Connected && !wsIsInit;

    if (shouldConnect)
    {
        NDBG("Start HTTP Server");
        server.begin();
        NDBG("HTTP server started");

        wsIsInit = true;

        NDBG("WebSocket server established");
    }
    else
    {
        closeServer();
    }
}

void WebServerComponent::closeServer()
{
    NDBG("Closing WebSocket connections");
    if (!wsIsInit)
    {
        NDBG("WebSocket is not initialized");
        return;
    }

    ws.closeAll();

    NDBG("WebSocket connections closed");
}

void WebServerComponent::handleFileUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
    NDBG("Server File upload Client:" + request->client()->remoteIP().toString() + " " + String(request->url()) + "Filename: " + filename + ", Index: " + String(index) + ", Length: " + String(len) + ", Final: " + String(final));

#ifdef USE_FILES
    if (index == 0)
    {
        // First chunk of a new upload, find a free slot
        int i;
        for (i = 0; i < MAX_CONCURRENT_UPLOADS; i++)
        {
            if (uploadingFiles[i].request == nullptr)
            {
                uploadingFiles[i].request = request;
                break;
            }
        }

        // If no free slot was found, abort
        if (i == MAX_CONCURRENT_UPLOADS)
        {
            DBG("Upload Error: No free slot for new upload!");
            return;
        }

        String dest = "";
        if (filename.endsWith(".wasm"))
            dest = "/scripts";
        else if (filename.endsWith(".colors") || filename.endsWith(".meta"))
            dest = "/playback";
        else if (filename.endsWith(".seq"))
            dest = "/playback";
        if (dest == "")
            dest = request->hasArg("folder") ? request->arg("folder") : "";
        dest += "/" + filename;

        DBG("Upload Start: " + String(dest));
        uploadingFiles[i].file = FilesComponent::instance->openFile(dest, true, true);

        // Add a disconnect handler to clean up if the client aborts
        request->onDisconnect([this, request]()
                              {
            for (int i = 0; i < MAX_CONCURRENT_UPLOADS; i++) {
                if (uploadingFiles[i].request == request) {
                    DBG("Upload client disconnected. Cleaning up file.");
                    uploadingFiles[i].file.close();
                    uploadingFiles[i].request = nullptr; // Free the slot
                    break;
                }
            } });
    }

    // Find the correct slot for this request and write data
    for (int i = 0; i < MAX_CONCURRENT_UPLOADS; i++)
    {
        if (uploadingFiles[i].request == request)
        {
            if (len > 0 && uploadingFiles[i].file)
            {
                uploadingFiles[i].file.write(data, len);
                yield();
            }

            if (final)
            {
                DBG("Upload Complete: " + String(uploadingFiles[i].file.name()) + ", size: " + String(index + len));
                uploadingFiles[i].file.close();
                uploadingFiles[i].request = nullptr; // Free the slot
            }
            break;
        }
    }
#endif
}

void WebServerComponent::onAsyncWSEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
                                        void *arg, uint8_t *data, size_t len)
{
    // DBG("WS Event");
    switch (type)
    {
    case WS_EVT_CONNECT:
        DBG("WebSocket client " + String(client->id()) + "connected from " + String(client->remoteIP().toString()));
        break;
    case WS_EVT_DISCONNECT:
        DBG("WebSocket client " + String(client->id()) + " disconnected");
        break;
    case WS_EVT_DATA:
        handleWebSocketMessage(arg, data, len);
        break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
        break;
    }
}

void WebServerComponent::handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    if (info->final && info->index == 0 && info->len == len)
    {
        data[len] = 0;
        if (info->opcode == WS_TEXT)
            parseTextMessage(String((char *)data));
        else if (info->opcode == WS_BINARY)
            parseBinaryMessage(data, len);
    }
}

void WebServerComponent::parseTextMessage(String msg)
{
    DBG("Text message: " + msg);
}

void WebServerComponent::parseBinaryMessage(uint8_t *data, size_t len)
{
#ifdef USE_OSC
    OSCMessage msg;
    msg.fill(data, len);
    if (!msg.hasError())
    {
        // DBG("Got websocket OSC message");

        char addr[64];
        msg.getAddress(addr);
        tmpExcludeParam = String(addr);

        OSCComponent::instance->processMessage(msg);

        tmpExcludeParam = "";
    }
#endif
}

void WebServerComponent::sendParamFeedback(Component *c, String pName, var *data, int numData)
{
#ifdef USE_OSC
    OSCMessage msg = OSCComponent::createMessage(c->getFullPath(), pName, data, numData, false);

    char addr[64];
    msg.getAddress(addr);
    if (String(addr) == tmpExcludeParam)
        return;

    wsPrint.flush();
    msg.send(wsPrint);

    if (!ws.availableForWriteAll())
        return;
    ws.binaryAll(wsPrint.data, wsPrint.index);
#endif
}

void WebServerComponent::sendBye(String type)
{
#ifdef USE_OSC
    OSCMessage msg("/bye");
    msg.add(type.c_str());

    wsPrint.flush();
    msg.send(wsPrint);

    if (!ws.availableForWriteAll())
        return;
    ws.binaryAll(wsPrint.data, wsPrint.index);
#endif
}