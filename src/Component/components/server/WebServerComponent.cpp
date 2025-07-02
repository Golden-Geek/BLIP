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

#ifdef USE_ASYNC_WEBSOCKET
    server.addHandler(&ws);
    ws.onEvent(std::bind(&WebServerComponent::onAsyncWSEvent, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
#else
    ws.onEvent(std::bind<void>(&WebServerComponent::onWSEvent, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
#endif

#ifdef USE_OSC
    server.on("/", HTTP_GET, [&](AsyncWebServerRequest *request)
              {

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
#ifndef USE_ASYNC_WEBSOCKET
            o["WS_PORT"] = 81;
#endif

            String jStr;
            serializeJson(doc, jStr);
            request->send(200, "application/json", jStr);
        }
        else
        {
            std::shared_ptr<bool> showConfig = std::make_shared<bool>(request->hasArg("config")?request->arg("config")=="1":true);
            std::shared_ptr<OSCQueryChunk> chunk = std::make_shared<OSCQueryChunk>(OSCQueryChunk(RootComponent::instance));
            AsyncWebServerResponse *response = request->beginChunkedResponse("application/json", [chunk, showConfig](uint8_t *buffer, size_t maxLen, size_t index) {
 
                if(chunk->nextComponent == nullptr) return 0;

                // DBG("Fill chunk, config = " + String(*showConfig));

                chunk->nextComponent->fillChunkedOSCQueryData(chunk.get(), *showConfig);
                
                
                sprintf((char*)buffer, chunk->data.c_str());
                return (int)chunk->data.length();
                
            });

        request->send(response);
        } });
#endif

    // server.onNotFound(std::bind(&WebServerComponent::handleNotFound, this));

    // server.on("/", HTTP_ANY, std::bind(&WebServerComponent::handleQueryData, this));
    // server.on("/settings", HTTP_ANY, std::bind(&WebServerComponent::handleSettings, this));
    // server.on("/uploadFile", HTTP_POST, std::bind(&WebServerComponent::returnOK, this), std::bind(&WebServerComponent::handleFileUpload, this));

#ifdef USE_FILES
    server.on(
        "/uploadFile", HTTP_POST, [](AsyncWebServerRequest *request)
        { request->send(200); },
        std::bind(&WebServerComponent::handleFileUpload,
                  this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));

    if (FilesComponent::instance->useInternalMemory)
    {
        server.serveStatic("/edit", LittleFS, "/server/edit.html");
        server.serveStatic("/upload", LittleFS, "/server/upload.html");
        server.serveStatic("/server/", LittleFS, "/server");
    }
    else
    {
        server.serveStatic("/edit", FilesComponent::fs, "/server/edit.html");
        server.serveStatic("/upload", FilesComponent::fs, "/server/upload.html");
        server.serveStatic("/server/", FilesComponent::fs, "/server");
    }
#endif

    return true;
}

void WebServerComponent::updateInternal()
{
#ifdef USE_ASYNC_WEBSOCKET
    // server.handleClient();
    ws.cleanupClients();
#else
    ws.loop();
#endif
}

void WebServerComponent::clearInternal()
{
#ifdef USE_ASYNC_WEBSOCKET
    ws.closeAll();
#else
    ws.broadcastTXT("close");
    // DBG("Before disconnect, remaining: " + String(ws.connectedClients()));
    ws.disconnect();
    // DBG("Disconnecting WebSocket clients, remaining: " + String(ws.connectedClients()));
    delay(500);
#endif
}

// SERVER
void WebServerComponent::onEnabledChanged()
{
    setupConnection();
}

void WebServerComponent::setupConnection()
{
    bool shouldConnect = enabled && WifiComponent::instance->state == WifiComponent::Connected;

    if (shouldConnect)
    {
        NDBG("Start HTTP Server");
        server.begin();
        NDBG("HTTP server started");

#ifndef USE_ASYNC_WEBSOCKET
        ws.begin();
#endif
    }
    else
    {
        // server.stop();
        // NDBG("HTTP server closed");
    }
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

#ifdef USE_ASYNC_WEBSOCKET
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
        // if (strcmp((char*)data, "toggle") == 0) {
        //   ledState = !ledState;
        //   notifyClients();
        // }

        if (info->opcode == WS_TEXT)
        {
            DBG("Message : " + String((char *)data));
        }
        else if (info->opcode == WS_BINARY)
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
    }
}
#else
void WebServerComponent::onWSEvent(uint8_t id, WStype_t type, uint8_t *data, size_t len)
{

    switch (type)
    {
    case WStype_DISCONNECTED:
        DBG("WebSocket client " + String(id) + " disconnected from " + StringHelpers::ipToString(ws.remoteIP(id)));
        break;
    case WStype_CONNECTED:
        DBG("WebSocket client " + String(id) + " connected from " + StringHelpers::ipToString(ws.remoteIP(id)) + ", num connected " + String(ws.connectedClients()));
        // webSocket.sendTXT(id, "Connected");
        break;

    case WStype_TEXT:
        DBG("Text received from " + String(id) + " > " + String((char *)data));

        // send message to client
        // webSocket.sendTXT(num, "message here");

        // send data to all connected clients
        // webSocket.broadcastTXT("message here");
        break;

    case WStype_BIN:
    {

        // DBG("Got binary");
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
    }
#endif
    // send message to client
    // webSocket.sendBIN(num, payload, length);
    break;

    case WStype_ERROR:
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
    case WStype_PING:
    case WStype_PONG:
        break;

    default:
        break;
    }
}
#endif

// void WebServerComponent::sendParameterFeedback(Component *c, Parameter *param)
// {
// #ifdef USE_OSC
//     var v = param->getOSCQueryFeedbackData();
//     OSCMessage msg = OSCComponent::createMessage(c->getFullPath(), param->name, &v, 1, false);

//     char addr[64];
//     msg.getAddress(addr);
//     if (String(addr) == tmpExcludeParam)
//         return;

//     wsPrint.flush();
//     msg.send(wsPrint);
//     ws.binaryAll(wsPrint.data, wsPrint.index);
// #endif
// }

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

#ifdef USE_ASYNC_WEBSOCKET
    if (!ws.availableForWriteAll())
        return;
    ws.binaryAll(wsPrint.data, wsPrint.index);
#else
    ws.broadcastBIN(wsPrint.data, wsPrint.index);
#endif
#endif
}