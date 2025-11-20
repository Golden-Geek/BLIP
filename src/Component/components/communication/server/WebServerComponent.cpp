#include "UnityIncludes.h"
#include "WebServerComponent.h"
#include "FirstrunPage.h"

namespace
{
    bool pathLooksLikeEditAsset(const String &path)
    {
        if (path.length() == 0)
        {
            return false;
        }

        String lower = path;
        lower.toLowerCase();
        return lower.endsWith("/edit") || lower.endsWith("/edit.html") || lower.endsWith("/edit.html.gz") || lower.endsWith("edit");
    }

    String urlEncode(const String &value)
    {
        String encoded;
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
        String deviceIP = WifiComponent::instance ? WifiComponent::instance->getIP() : String("[noip]");
        if (deviceIP == "[noip]")
        {
            request->send(500, "text/plain", "Device IP unavailable; cannot bootstrap dashboard.");
            return;
        }

        while (deviceIP.endsWith("/"))
        {
            deviceIP.remove(deviceIP.length() - 1);
        }

        String deviceBase = "http://" + deviceIP;
        String destination = String("/firstrun?device=") + urlEncode(deviceBase);
        request->redirect(destination);
    }
}

ImplementSingleton(WebServerComponent);

void WebServerComponent::setupInternal(JsonObject o)
{
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
            o["DEVICE_TYPE"] = DeviceType;
            o["DEVICE_ID"] = DeviceID;
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
    server.on("/firstrun", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                  AsyncWebServerResponse *response = request->beginResponse(200, "text/html", BlipServerPages::FIRSTRUN_PAGE_GZ, BlipServerPages::FIRSTRUN_PAGE_GZ_LEN);
                  response->addHeader("Cache-Control", "no-store");
                  response->addHeader("Content-Encoding", "gzip");
                  request->send(response);
              });

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

        auto openRegularFile = [](const String &path) -> File {
            File candidate = FilesComponent::instance->openFile(path, false, false);
            if(!candidate) {
                return File();
            }
            if(candidate.isDirectory()) {
                candidate.close();
                return File();
            }
            return candidate;
        };

        String resolvedPath = request->url();
        String lastTriedPath = resolvedPath;
        File f = openRegularFile(resolvedPath);

        bool firstRunRedirectCandidate = pathLooksLikeEditAsset(resolvedPath);

        if(!f) {
            String path = resolvedPath;
            if(!path.startsWith("/server/")) {
                path = String("/server") + (path.startsWith("/")?"":"/") + path;
                lastTriedPath = path;
                f = openRegularFile(path);
                if(f) {
                    resolvedPath = path;
                } else if(!path.endsWith(".html") && !path.endsWith(".htm") && !path.endsWith(".css") && !path.endsWith(".js")) {
                    path += ".html";
                    lastTriedPath = path;
                    f = openRegularFile(path);
                    if(f) {
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
            String header = request->header("Accept-Encoding");
            header.toLowerCase();
            return header.indexOf("gzip") >= 0;
        };

        String logicalPath = f ? resolvedPath : lastTriedPath;
        bool usingCompressed = false;
        if(clientAcceptsGzip() && logicalPath.length() > 0) {
            auto tryGzipPath = [&](const String &candidate) -> File {
                return openRegularFile(candidate);
            };

            String sameDirGz = logicalPath + ".gz";
            File gzFile = tryGzipPath(sameDirGz);

            if(!gzFile && logicalPath.startsWith("/server/")) {
                const size_t serverPrefixLen = 7; // length of "/server"
                String relative = logicalPath.substring(serverPrefixLen);
                String altPath = String("/server-compressed") + relative + ".gz";
                gzFile = tryGzipPath(altPath);
            }

            if(gzFile) {
                if(f) {
                    f.close();
                }
                f = gzFile;
                usingCompressed = true;
            }
        }

        if(!f) {
            if(firstRunRedirectCandidate) {
                redirectToFirstRunPage(request);
                return;
            }
            request->send(404, "text/plain", "File not found : " + request->url()+" (also tried "+lastTriedPath+")");
            return;
        }

        auto determineContentType = [](const String &name) {
            if (name.endsWith(".html") || name.endsWith(".htm"))
            {
                return String("text/html");
            }
            if (name.endsWith(".css"))
            {
                return String("text/css");
            }
            if (name.endsWith(".js"))
            {
                return String("application/javascript");
            }
            if (name.endsWith(".json"))
            {
                return String("application/json");
            }
            if (name.endsWith(".svg"))
            {
                return String("image/svg+xml");
            }
            if (name.endsWith(".png"))
            {
                return String("image/png");
            }
            if (name.endsWith(".jpg") || name.endsWith(".jpeg"))
            {
                return String("image/jpeg");
            }
            if (name.endsWith(".ico"))
            {
                return String("image/x-icon");
            }
            return String("application/octet-stream");
        };

        String contentType = determineContentType(logicalPath.length() > 0 ? logicalPath : resolvedPath);

        DBG("Serving file: " + String(f.name()) + " size: " + String(f.size()) + (usingCompressed ? " (gzip)" : ""));

        AsyncWebServerResponse *response = request->beginChunkedResponse(contentType, [f](uint8_t *buffer, size_t maxLen, size_t index) mutable {

            size_t len = f.read(buffer, maxLen);
            if(len == 0) {
                f.close();
            }
            return len;
        });

        if(usingCompressed) {
            response->addHeader("Content-Encoding", "gzip");
        }

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

        String destFolder = "";
        if (filename.endsWith(".wasm") || filename.endsWith(".wmeta"))
            destFolder = "/scripts";
        else if (filename.endsWith(".colors") || filename.endsWith(".meta"))
            destFolder = "/playback";
        else if (filename.endsWith(".seq"))
            destFolder = "/playback";

        if (destFolder == "")
            destFolder = request->hasArg("folder") ? request->arg("folder") : "";

        FilesComponent::instance->createFolderIfNotExists(destFolder);
        String dest = destFolder + "/" + filename;

        NDBG("File Upload start from" + request->client()->remoteIP().toString() + ", at : " + String(request->url()) + "; Filename: " + filename + ", Length: " + String(len / 1024) + " kb");

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
                if (written == 0) {
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
                NDBG("Upload Complete: " + String(uploadingFiles[i].file.name()) + ", size: " + String(index + len));
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
    if (!sendFeedback)
        return;
    sendParamFeedback(c->getFullPath(), pName, data, numData);
}

void WebServerComponent::sendParamFeedback(String path, String pName, var *data, int numData)
{
    if (!sendFeedback)
        return;
#ifdef USE_OSC
    OSCMessage msg = OSCComponent::createMessage(path, pName, data, numData, false);

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

void WebServerComponent::sendDebugLog(const String &msg, String source, String type)
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
    String sanitizedMsg = msg;
    //escape newlines and backslashes and quotes for JSON
    sanitizedMsg.replace("\\", "\\\\");
    sanitizedMsg.replace("\n", "\\n");
    sanitizedMsg.replace("\"", "\\\"");
    ws.textAll("{\"COMMAND\":\"LOG\",\"DATA\":{\"type\":\"" + type + "\",\"source\":\"" + source + "\",\"message\":\"" + sanitizedMsg + "\"}}");
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
