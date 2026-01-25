#include "UnityIncludes.h"
#include "FirstrunPage.h"

namespace
{
    bool pathLooksLikeEditAsset(const std::string &path)
    {
        if (path.length() == 0)
        {
            return false;
        }

        std::string lower = StringHelpers::toLowerCase(path);
        return lower.ends_with("/edit") || lower.ends_with("/edit.html") || lower.ends_with("/edit.html.gz") || lower.ends_with("/edit");
    }

    std::string urlEncode(const std::string &value)
    {
        std::string encoded;
        encoded.reserve(value.length() * 3);
        const char *hex = "0123456789ABCDEF";
        for (size_t i = 0; i < value.length(); ++i)
        {
            const char c = value[i];
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~')
            {
                encoded += c;
            }
            else
            {
                const uint8_t byte = static_cast<uint8_t>(c);
                encoded += '%';
                encoded += hex[(byte >> 4) & 0x0F];
                encoded += hex[byte & 0x0F];
            }
        }
        return encoded;
    }

    void redirectToFirstRunPage(AsyncWebServerRequest *request)
    {
        std::string deviceIP = WifiComponent::instance ? WifiComponent::instance->getIP() : std::string("[noip]");
        if (deviceIP == "[noip]")
        {
            request->send(500, "text/plain", "Device IP unavailable; cannot bootstrap dashboard.");
            return;
        }

        while (deviceIP.ends_with("/"))
        {
            deviceIP.erase(deviceIP.length() - 1);
        }

        std::string deviceBase = "http://" + deviceIP;
        std::string destination = std::string("/firstrun?device=") + urlEncode(deviceBase);
        request->redirect(destination.c_str());
    }
}

ImplementSingleton(WebServerComponent);

void WebServerComponent::setupInternal(JsonObject o)
{
    updateRate = 1; // 2 Hz update rate, only for cleanup clients

    for (int i = 0; i < MAX_CONCURRENT_UPLOADS; i++)
    {
        uploadingFiles[i].request = nullptr;
    }

    AddBoolParamConfig(sendFeedback);
    AddBoolParamConfig(sendDebugLogs);
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
            StaticJsonDocument<1000> doc;
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
            o["DEVICE_TYPE"] = DeviceType;
            o["DEVICE_ID"] = DeviceID;
            o["OSC_PORT"] = OSC_LOCAL_PORT;
            o["OSC_TRANSPORT"] = "UDP";

            std::string jStr;
            serializeJson(doc, jStr);

            request->send(200, "application/json", jStr.c_str());
        }
        else
        {
            std::shared_ptr<bool> showConfig = std::make_shared<bool>(request->hasArg("config") ? request->arg("config") == "1" : true);
            std::shared_ptr<OSCQueryChunk> chunk = std::make_shared<OSCQueryChunk>(OSCQueryChunk(RootComponent::instance));

            struct ChunkStreamState
            {
                size_t expectedIndex = 0;
                size_t offsetInChunk = 0;
                uint8_t emptyRefills = 0;
                std::string current;
            };
            std::shared_ptr<ChunkStreamState> state = std::make_shared<ChunkStreamState>();

            AsyncWebServerResponse *response = request->beginChunkedResponse(
                "application/json",
                [chunk, showConfig, state](uint8_t *buffer, size_t maxLen, size_t index)
                {
                    if (maxLen == 0)
                    {
                        return (size_t)0;
                    }

                    // Resync if the server calls us with an unexpected index.
                    if (index != state->expectedIndex)
                    {
                        state->expectedIndex = index;
                        state->offsetInChunk = 0;
                        state->current = "";
                    }

                    // Refill chunk data when we've exhausted the current chunk.
                    while (state->offsetInChunk >= state->current.length())
                    {
                        if (chunk->nextComponent == nullptr)
                        {
                            return (size_t)0;
                        }

                        chunk->nextComponent->fillChunkedOSCQueryData(chunk.get(), *showConfig);
                        state->current = chunk->data;
                        state->offsetInChunk = 0;

                        if (state->current.length() == 0)
                        {
                            if (++state->emptyRefills >= 4)
                            {
                                return (size_t)0;
                            }
                        }
                        else
                        {
                            state->emptyRefills = 0;
                        }
                    }

                    const size_t remaining = state->current.length() - state->offsetInChunk;
                    const size_t toCopy = remaining < maxLen ? remaining : maxLen;
                    memcpy(buffer, state->current.c_str() + state->offsetInChunk, toCopy);
                    state->offsetInChunk += toCopy;
                    state->expectedIndex += toCopy;
                    return toCopy;
                });

            request->send(response);
        } });

#ifdef USE_FILES
    NDBG("Setting up local files to serve");
    server.on("/firstrun", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                  AsyncWebServerResponse *response = request->beginResponse(200, "text/html", BlipServerPages::FIRSTRUN_PAGE_GZ, BlipServerPages::FIRSTRUN_PAGE_GZ_LEN);
                  response->addHeader("Cache-Control", "no-store");
                  response->addHeader("Content-Encoding", "gzip");
                  request->send(response); });

    server.on(
        "/uploadFile", HTTP_POST, [](AsyncWebServerRequest *request)
        { 
            AsyncWebServerResponse *response = request->beginResponse(200);
            response->addHeader("Access-Control-Allow-Origin", "*");
            request->send(response); },
        std::bind(&WebServerComponent::handleFileUpload,
                  this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));

    server.onNotFound([](AsyncWebServerRequest *request)
                      {

        if(!FilesComponent::instance->isInit) {
            request->send(500, "text/plain", "Filesystem not initialized");
            return;
        }

        auto isRegularFile = [](const std::string &path) -> bool {
            File candidate = FilesComponent::instance->openFile(path, false, false);
            if(!candidate) {
                return false;
            }
            const bool ok = !candidate.isDirectory();
            candidate.close();
            return ok;
        };

        std::string resolvedPath = request->url().c_str();
        std::string lastTriedPath = resolvedPath;
        bool found = isRegularFile(resolvedPath);

        bool firstRunRedirectCandidate = pathLooksLikeEditAsset(resolvedPath);

        if(!found) {
            std::string path = resolvedPath;
            if(!path.starts_with("/server/")) {
                path = std::string("/server") + (path.starts_with("/")?"":"/") + path;
                lastTriedPath = path;
                found = isRegularFile(path);
                if(found) {
                    resolvedPath = path;
                } else if(!path.ends_with(".html") && !path.ends_with(".htm") && !path.ends_with(".css") && !path.ends_with(".js")) {
                    path += ".html";
                    lastTriedPath = path;
                    found = isRegularFile(path);
                    if(found) {
                        resolvedPath = path;
                    }
                }
            }
            firstRunRedirectCandidate = firstRunRedirectCandidate || pathLooksLikeEditAsset(lastTriedPath);
        }

        const auto clientAcceptsGzip = [&]() {
            if(!request->hasHeader("Accept-Encoding")) {
                return false;
            }
            String headerStr = request->header("Accept-Encoding");
            headerStr.toLowerCase();
            std::string header = headerStr.c_str();
            return header.find("gzip") != std::string::npos;
        };

        const std::string logicalPath = found ? resolvedPath : lastTriedPath;
        std::string pathToServe = found ? resolvedPath : logicalPath;
        bool usingCompressed = false;

        // Try serving gzip even if the plain file doesn't exist (some deployments only ship *.gz).
        if(clientAcceptsGzip() && logicalPath.length() > 0) {
            const std::string sameDirGz = logicalPath + ".gz";
            if(isRegularFile(sameDirGz)) {
                pathToServe = sameDirGz;
                usingCompressed = true;
                found = true;
            } else if(logicalPath.starts_with("/server/")) {
                const size_t serverPrefixLen = 7; // length of "/server"
                const std::string relative = logicalPath.substr(serverPrefixLen);
                const std::string altPath = std::string("/server-compressed") + relative + ".gz";
                if(isRegularFile(altPath)) {
                    pathToServe = altPath;
                    usingCompressed = true;
                    found = true;
                }
            }
        }

        if(!found) {
            if(firstRunRedirectCandidate) {
                redirectToFirstRunPage(request);
                return;
            }
            std::string msg = "File not found : " + std::string(request->url().c_str()) +" (also tried " + lastTriedPath + ")";
            request->send(404, "text/plain", msg.c_str());
            return;
        }

        auto determineContentType = [](const std::string &name) {
            if (name.ends_with(".html") || name.ends_with(".htm"))
            {
                return std::string("text/html");
            }
            if (name.ends_with(".css"))
            {
                return std::string("text/css");
            }
            if (name.ends_with(".js"))
            {
                return std::string("application/javascript");
            }
            if (name.ends_with(".json"))
            {
                return std::string("application/json");
            }
            if (name.ends_with(".svg"))
            {
                return std::string("image/svg+xml");
            }
            if (name.ends_with(".png"))
            {
                return std::string("image/png");
            }
            if (name.ends_with(".jpg") || name.ends_with(".jpeg"))
            {
                return std::string("image/jpeg");
            }
            if (name.ends_with(".ico"))
            {
                return std::string("image/x-icon");
            }
            return std::string("application/octet-stream");
        };

        std::string contentType = determineContentType(logicalPath.length() > 0 ? logicalPath : resolvedPath);

        DBG("Serving file: " + pathToServe + (usingCompressed ? " (gzip)" : ""));

        AsyncWebServerResponse *response = request->beginResponse(FilesComponent::instance->getFS(), pathToServe.c_str(), contentType.c_str(), false);
        if(!response) {
            request->send(500, "text/plain", "Failed to create file response");
            return;
        }

        if(usingCompressed) {
            response->addHeader("Content-Encoding", "gzip");
        }

        DBG("Serving file response");
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
            ws.cleanupClients(4);
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

#ifdef USE_FILES

#ifdef USE_OTA
    if (filename == "firmware.bin")
    {
        handleOTAUpload(request, index, data, len, final);
        return;
    }
#endif

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

        std::string destFolder = "";
        if (filename.endsWith(".wasm") || filename.endsWith(".wmeta"))
            destFolder = "/scripts";
        else if (filename.endsWith(".colors") || filename.endsWith(".meta"))
            destFolder = "/playback";
        else if (filename.endsWith(".seq"))
            destFolder = "/playback";

        if (destFolder == "")
            destFolder = request->hasArg("folder") ? request->arg("folder").c_str() : "";

        FilesComponent::instance->createFolderIfNotExists(destFolder);
        std::string dest = destFolder + "/" + filename.c_str();

        NDBG("File Upload start from" + std::string(request->client()->remoteIP().toString().c_str()) + ", at : " + request->url().c_str() + "; Filename: " + filename.c_str() + ", Length: " + std::to_string(len / 1024) + " kb");

        uploadingFiles[i].file = FilesComponent::instance->openFile(dest, true, true);

        // Add a disconnect handler to clean up if the client aborts
        request->onDisconnect([this, request]()
                              {
            for (int i = 0; i < MAX_CONCURRENT_UPLOADS; i++)
            {
                if (uploadingFiles[i].request == request)
                {
                    NDBG("Upload client disconnected. Cleaning up file.");
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
                size_t written = uploadingFiles[i].file.write(data, len);
                if (written == 0)
                {
                    NDBG("Upload Aborted: Write error");
                    uploadingFiles[i].request->client()->close();
                }

#ifdef FILES_TYPE_FLASH
                delayMicroseconds(500);
#endif
                yield();
            }

            if (final)
            {
                NDBG("Upload Complete: " + std::string(uploadingFiles[i].file.name()) + ", size: " + std::to_string(index + len));
                uploadingFiles[i].file.flush();
                uploadingFiles[i].file.close();
                uploadingFiles[i].request = nullptr; // Free the slot
            }
            break;
        }
    }
#endif
}

#ifdef USE_OTA
void WebServerComponent::handleOTAUpload(AsyncWebServerRequest *request, size_t index, uint8_t *data, size_t len, bool final)
{
    if (index == 0)
    {
        NDBG("OTA Update Start");
        if (!Update.begin(UPDATE_SIZE_UNKNOWN))
        {
            NDBG("OTA Update Begin Failed");
            return;
        }
    }

    if (len > 0)
    {
        size_t written = Update.write(data, len);
        if (written != len)
        {
            NDBG("OTA Update Write Failed");
            return;
        }
    }

    if (final)
    {
        if (Update.end(true))
        {
            NDBG("OTA Update Complete. Rebooting...");
            RootComponent::instance->restart();
        }
        else
        {
            NDBG("OTA Update End Failed");
        }
    }
}
#endif

void WebServerComponent::onAsyncWSEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
                                        void *arg, uint8_t *data, size_t len)
{
    // DBG("WS Event");
    switch (type)
    {
    case WS_EVT_CONNECT:
        DBG("WebSocket client " + std::to_string(client->id()) + "connected from " + client->remoteIP().toString().c_str());
        break;
    case WS_EVT_DISCONNECT:
        DBG("WebSocket client " + std::to_string(client->id()) + " disconnected");
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
        if (info->opcode == WS_TEXT)
        {
            std::string text;
            text.reserve(len + 1);
            text.append((const char *)data, len);
            parseTextMessage(text);
        }
        else if (info->opcode == WS_BINARY)
            parseBinaryMessage(data, len);
    }
}

void WebServerComponent::parseTextMessage(std::string msg)
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
        tmpExcludeParam = std::string(addr);

        OSCComponent::instance->processMessage(msg);

        tmpExcludeParam = "";
    }
#endif
}

void WebServerComponent::sendParamFeedback(Component *c, std::string pName, var *data, int numData)
{
    if (!sendFeedback)
        return;
    sendParamFeedback(c->getFullPath(), pName, data, numData);
}

void WebServerComponent::sendParamFeedback(std::string path, std::string pName, var *data, int numData)
{
    if (!sendFeedback)
        return;
#ifdef USE_OSC
    OSCMessage msg = OSCComponent::createMessage(path, pName, data, numData, false);

    char addr[64];
    msg.getAddress(addr);
    if (std::string(addr) == tmpExcludeParam)
        return;

    wsPrint.flush();
    msg.send(wsPrint);

    if (!ws.availableForWriteAll())
        return;

    ws.binaryAll(wsPrint.data, wsPrint.index);
#endif
}

void WebServerComponent::sendDebugLog(const std::string &msg, std::string source, std::string type)
{
    if (!sendDebugLogs)
        return;

    /* message is like this :
        {"COMMAND":"LOG",
        "DATA": {
        "type":"info",
        "source":"ComponentName",
        "message":"This is a debug log message"
        }
    */
    std::string sanitizedMsg;
    sanitizedMsg.reserve(msg.size());
    // escape newlines and backslashes and quotes for JSON
    for (char ch : msg)
    {
        if (ch == '\\')
        {
            sanitizedMsg += "\\\\";
        }
        else if (ch == '\n')
        {
            sanitizedMsg += "\\n";
        }
        else if (ch == '"')
        {
            sanitizedMsg += "\\\"";
        }
        else
        {
            sanitizedMsg += ch;
        }
    }
    std::string payload = "{\"COMMAND\":\"LOG\",\"DATA\":{\"type\":\"" + type + "\",\"source\":\"" + source + "\",\"message\":\"" + sanitizedMsg + "\"}}";
    ws.textAll(payload.c_str());
}

void WebServerComponent::sendBye(std::string type)
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
