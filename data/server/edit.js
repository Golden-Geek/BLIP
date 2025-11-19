// ==============================
// Connection & configuration
// ==============================
const searchParams = new URLSearchParams(window.location.search);
const urlIp = searchParams.get("ip");
const local = window.location.hostname.startsWith("1") || window.location.hostname.includes(".local");
const ip = urlIp || (local ? window.location.hostname : "192.168.1.193");
let data = {};
let oscWS;
let connectAttempts = 0;

const firmwareBaseUrl = "https://www.goldengeek.org/blip/download/firmware/getFirmwares.php";
const firmwareListUrl = firmwareBaseUrl + "?list";
let firmwareCatalog = null;
let deviceInfo = null;
let versionCheckStarted = false;
let catalogMatchKey = null;
let catalogMatchEntry = null;
let pendingAutoVersion = null;
let uploadInProgress = false;
let autoUpdateInProgress = false;
let uploadFirmwareBlob = null;
let settingsDeviceType = null;
let settingsDeviceName = null;
let settingsPropId = null;

const firmwareEls = {
    current: document.getElementById("currentFirmwareVersion"),
    latest: document.getElementById("latestFirmwareVersion"),
    pill: document.getElementById("firmwareStatusPill"),
    message: document.getElementById("firmwareStatusMessage"),
};

const firmwareDownloadEls = {
    select: document.getElementById("firmwareVersionSelect"),
    button: document.getElementById("downloadFirmwareBtn"),
    status: document.getElementById("downloadFirmwareStatus"),
};

const autoUpdateEls = {
    button: document.getElementById("autoUpdateBtn"),
    status: document.getElementById("autoUpdateStatus"),
};

const deviceMetaEls = {
    ip: document.getElementById("deviceMetaIp"),
    id: document.getElementById("deviceMetaId"),
    name: document.getElementById("deviceMetaName"),
    type: document.getElementById("deviceMetaType"),
    prop: document.getElementById("deviceMetaProp"),
};

updateDeviceMetaDisplay();

if (firmwareDownloadEls.button) {
    firmwareDownloadEls.button.addEventListener("click", handleFirmwareDownload);
}

if (autoUpdateEls.button) {
    autoUpdateEls.button.addEventListener("click", handleAutoUpdate);
}

setAutoUpdateAvailability(null, "Auto-update will enable when a newer firmware is available.");

setTimeout(() => {
    connectToServer();
    startVersionChecks();
}, 100);

function startVersionChecks() {
    if (versionCheckStarted) return;
    versionCheckStarted = true;
    setDownloadStatus("Matching device with firmware catalog…");
    setAutoUpdateAvailability(null, "Matching device with firmware catalog…");
    fetchDeviceHostInfo();
    fetchFirmwareCatalog();
}

function getCatalogDeviceType() {
    if (deviceInfo && deviceInfo.DEVICE_TYPE) {
        return deviceInfo.DEVICE_TYPE;
    }
    if (settingsDeviceType) {
        return settingsDeviceType;
    }
    return null;
}

function updateDeviceMetaDisplay() {
    if (deviceMetaEls.ip) {
        deviceMetaEls.ip.textContent = ip || "—";
    }

    if (deviceMetaEls.id) {
        deviceMetaEls.id.textContent = (deviceInfo && deviceInfo.DEVICE_ID) || "—";
    }

    if (deviceMetaEls.name) {
        const nameValue = (deviceInfo && deviceInfo.NAME) || settingsDeviceName || "—";
        deviceMetaEls.name.textContent = nameValue;
    }

    if (deviceMetaEls.type) {
        deviceMetaEls.type.textContent = getCatalogDeviceType() || "—";
    }

    if (deviceMetaEls.prop) {
        const propValue = settingsPropId ?? (deviceInfo && deviceInfo.PROP_ID);
        deviceMetaEls.prop.textContent =
            propValue === null || propValue === undefined || propValue === ""
                ? "—"
                : propValue;
    }
}

function findParameterByPath(node, fullPath) {
    if (!node || !node.CONTENTS) return null;

    for (const key in node.CONTENTS) {
        const item = node.CONTENTS[key];
        if (!item) continue;

        if (item.FULL_PATH === fullPath) {
            return item;
        }

        if (item.CONTENTS) {
            const nested = findParameterByPath(item, fullPath);
            if (nested) {
                return nested;
            }
        }
    }

    return null;
}

function getSettingValue(structureRoot, path) {
    const param = findParameterByPath(structureRoot, path);
    if (!param) return null;
    const valueArray = Array.isArray(param.VALUE) ? param.VALUE : [param.VALUE];
    return valueArray && valueArray.length ? valueArray[0] : null;
}

function syncSettingsMetadata(structureRoot) {
    if (!structureRoot) return;

    let deviceTypeChanged = false;
    const typeFromSettings = getSettingValue(structureRoot, "/settings/deviceType");
    if (typeFromSettings && typeFromSettings !== settingsDeviceType) {
        settingsDeviceType = typeFromSettings;
        if (deviceInfo && !deviceInfo.DEVICE_TYPE) {
            deviceInfo.DEVICE_TYPE = typeFromSettings;
        }
        deviceTypeChanged = true;
    }

    const propValue = getSettingValue(structureRoot, "/settings/propID");
    if (propValue !== null && propValue !== undefined) {
        settingsPropId = propValue;
    }

    const deviceNameValue = getSettingValue(structureRoot, "/settings/deviceName");
    if (deviceNameValue) {
        settingsDeviceName = deviceNameValue;
    }

    updateDeviceMetaDisplay();

    if (deviceTypeChanged && deviceInfo) {
        evaluateFirmwareVersions();
    }
}

function updateFirmwareStatusUI({ current, latest, state, message }) {
    if (current !== undefined && firmwareEls.current) {
        firmwareEls.current.textContent = current || "—";
    }

    if (latest !== undefined && firmwareEls.latest) {
        firmwareEls.latest.textContent = latest || "—";
    }

    if (state && firmwareEls.pill) {
        firmwareEls.pill.classList.remove("ok", "warn", "error");
        let pillText = "";
        switch (state) {
            case "ok":
                firmwareEls.pill.classList.add("ok");
                pillText = "Up to date";
                break;
            case "warn":
                firmwareEls.pill.classList.add("warn");
                pillText = "Update available";
                break;
            case "error":
                firmwareEls.pill.classList.add("error");
                pillText = "Unavailable";
                break;
            default:
                pillText = "Checking…";
                break;
        }
        firmwareEls.pill.textContent = pillText;
    }

    if (message !== undefined && firmwareEls.message) {
        firmwareEls.message.textContent = message;
    }
}

function fetchDeviceHostInfo() {
    updateFirmwareStatusUI({ state: "checking", message: "Reading firmware version from device…" });
    fetch("http://" + ip + "/?HOST_INFO=1", { cache: "no-cache" })
        .then((response) => {
            if (!response.ok) {
                throw new Error("HOST_INFO request failed with status " + response.status);
            }
            return response.json();
        })
        .then((info) => {
            deviceInfo = info || {};
            if (deviceInfo.DEVICE_TYPE) {
                settingsDeviceType = deviceInfo.DEVICE_TYPE;
            }
            updateDeviceMetaDisplay();
            const detectedType = getCatalogDeviceType();
            const detectedMsg = detectedType
                ? "Detected " + detectedType + ". Checking online catalog…"
                : "Checking online catalog…";
            updateFirmwareStatusUI({ current: deviceInfo.VERSION || "—", message: detectedMsg });
            evaluateFirmwareVersions();
        })
        .catch((err) => {
            console.warn("Failed to get HOST_INFO", err);
            updateFirmwareStatusUI({ state: "error", message: "Unable to read firmware version from device." });
            updateDownloadControls(null, null);
            setDownloadStatus("Unable to read device info; firmware downloads are disabled.");
            setAutoUpdateAvailability(null, "Unable to read device info; auto-update disabled.");
        });
}

function fetchFirmwareCatalog() {
    fetch(firmwareListUrl, { cache: "no-cache" })
        .then((response) => {
            if (!response.ok) {
                throw new Error("Firmware list request failed with status " + response.status);
            }
            return response.json();
        })
        .then((list) => {
            firmwareCatalog = list || {};
            evaluateFirmwareVersions();
        })
        .catch((err) => {
            console.warn("Failed to fetch firmware catalog", err);
            updateFirmwareStatusUI({ state: "error", message: "Unable to reach firmware download server." });
            updateDownloadControls(null, null);
            setDownloadStatus("Firmware download server unreachable.");
            setAutoUpdateAvailability(null, "Firmware download server unreachable.");
        });
}

function evaluateFirmwareVersions() {
    if (!deviceInfo) {
        updateFirmwareStatusUI({ state: "checking", message: "Waiting for device info…" });
        setDownloadStatus("Waiting for device info to load…");
        return;
    }

    const currentVersion = deviceInfo.VERSION || "—";
    updateFirmwareStatusUI({ current: currentVersion });

    if (!firmwareCatalog) {
        updateFirmwareStatusUI({ state: "checking", message: "Waiting for firmware catalog…" });
        setDownloadStatus("Waiting for firmware catalog to load…");
        setAutoUpdateAvailability(null, "Waiting for firmware catalog to load…");
        return;
    }

    const catalogDeviceType = getCatalogDeviceType();
    if (!catalogDeviceType) {
        updateFirmwareStatusUI({ state: "checking", message: "Waiting for device type information…" });
        setDownloadStatus("Waiting for device type info before enabling downloads.");
        setAutoUpdateAvailability(null, "Waiting for device type info before enabling downloads.");
        return;
    }

    const match = findCatalogEntry(catalogDeviceType);
    const catalogEntry = match ? match.entry : null;
    if (!catalogEntry || !Array.isArray(catalogEntry.versions) || catalogEntry.versions.length === 0) {
        updateFirmwareStatusUI({ latest: "—", state: "error", message: "Device not found in firmware catalog." });
        updateDownloadControls(null, null);
        setDownloadStatus("Could not match this device to any downloadable firmware.");
        setAutoUpdateAvailability(null, "Could not match this device to any downloadable firmware.");
        return;
    }

    updateDownloadControls(catalogEntry, match.key);

    const latestVersion = catalogEntry.versions[0];
    updateFirmwareStatusUI({ latest: latestVersion || "—" });

    if (!deviceInfo.VERSION || !latestVersion) {
        updateFirmwareStatusUI({ state: "warn", message: "Device firmware version missing or catalog incomplete." });
        setAutoUpdateAvailability(null, "Device firmware version missing; auto-update disabled.");
        return;
    }

    const comparison = compareVersions(deviceInfo.VERSION, latestVersion);
    if (comparison >= 0) {
        updateFirmwareStatusUI({ state: "ok", message: "Firmware is up to date." });
        setDownloadStatus("Device already runs the latest firmware.");
        setAutoUpdateAvailability(null, "Device already up to date; auto-update disabled.");
    } else {
        const readableName = catalogEntry.name || catalogDeviceType || "device";
        updateFirmwareStatusUI({
            state: "warn",
            message: "Update available: install version " + latestVersion + " for " + readableName + ".",
        });
        setDownloadStatus("Download version " + latestVersion + " to update this device.");
        setAutoUpdateAvailability(latestVersion, "Auto-update ready for version " + latestVersion + ".");
    }
}

function findCatalogEntry(deviceName) {
    if (!firmwareCatalog) return null;
    const normalizedDevice = normalizeName(deviceName);
    if (!normalizedDevice) {
        return null;
    }
    let match = null;
    Object.keys(firmwareCatalog).some((key) => {
        const entry = firmwareCatalog[key];
        if (
            normalizeName(key) === normalizedDevice ||
            normalizeName(entry && entry.name) === normalizedDevice
        ) {
            match = { entry, key };
            return true;
        }
        return false;
    });
    return match;
}

function normalizeName(value) {
    if (!value) return "";
    return value.toString().toLowerCase().replace(/\s+/g, "").trim();
}

function compareVersions(a, b) {
    const sanitize = (version) =>
        version
            .split(/[.-]/)
            .map((part) => {
                const parsed = parseInt(part, 10);
                return Number.isNaN(parsed) ? 0 : parsed;
            });

    const partsA = sanitize(a || "0");
    const partsB = sanitize(b || "0");
    const maxLen = Math.max(partsA.length, partsB.length);

    for (let i = 0; i < maxLen; i++) {
        const diff = (partsA[i] || 0) - (partsB[i] || 0);
        if (diff !== 0) {
            return diff;
        }
    }
    return 0;
}

function updateDownloadControls(entry, key) {
    catalogMatchEntry = entry || null;
    catalogMatchKey = key || null;

    if (!firmwareDownloadEls.select || !firmwareDownloadEls.button) {
        return;
    }

    firmwareDownloadEls.select.innerHTML = "";
    firmwareDownloadEls.select.disabled = true;
    firmwareDownloadEls.button.disabled = true;

    if (!entry || !Array.isArray(entry.versions) || entry.versions.length === 0) {
        const option = document.createElement("option");
        option.value = "";
        option.textContent = entry ? "No versions available" : "Unavailable";
        firmwareDownloadEls.select.appendChild(option);
        setDownloadStatus(
            entry
                ? "This device currently has no downloadable firmware packages."
                : "Waiting for firmware catalog match before enabling downloads."
        );
        return;
    }

    entry.versions.forEach((version) => {
        const option = document.createElement("option");
        option.value = version;
        option.textContent = version;
        firmwareDownloadEls.select.appendChild(option);
    });

    if (!key) {
        setDownloadStatus("Matched device but missing catalog key; downloads unavailable.");
        setAutoUpdateAvailability(null, "Matched device but missing catalog key; auto-update unavailable.");
        return;
    }

    firmwareDownloadEls.select.disabled = false;
    firmwareDownloadEls.button.disabled = false;
    setDownloadStatus("Select a version and click download to grab the firmware .zip.");
}

function setDownloadStatus(message) {
    if (firmwareDownloadEls.status) {
        firmwareDownloadEls.status.textContent = message;
    }
}

function setAutoUpdateStatus(message) {
    if (autoUpdateEls.status) {
        autoUpdateEls.status.textContent = message;
    }
}

function refreshAutoUpdateButtonState() {
    if (!autoUpdateEls.button) return;
    if (autoUpdateInProgress || uploadInProgress) {
        autoUpdateEls.button.disabled = true;
    } else {
        autoUpdateEls.button.disabled = !pendingAutoVersion;
    }

    if (pendingAutoVersion) {
        autoUpdateEls.button.textContent = "Download & Install " + pendingAutoVersion;
    } else {
        autoUpdateEls.button.textContent = "Download & Install Latest";
    }
}

function setAutoUpdateAvailability(version, message) {
    pendingAutoVersion = version || null;
    if (message) {
        setAutoUpdateStatus(message);
    }
    refreshAutoUpdateButtonState();
}

function handleFirmwareDownload() {
    if (!catalogMatchKey || !catalogMatchEntry) {
        setDownloadStatus("Device has not been matched to a firmware catalog entry yet.");
        return;
    }

    if (!firmwareDownloadEls.select) {
        return;
    }

    const selectedVersion = firmwareDownloadEls.select.value;
    if (!selectedVersion) {
        setDownloadStatus("Select a firmware version first.");
        return;
    }

    const downloadUrl =
        firmwareBaseUrl +
        "?device=" +
        encodeURIComponent(catalogMatchKey) +
        "&version=" +
        encodeURIComponent(selectedVersion);

    setDownloadStatus("Opening download for version " + selectedVersion + "…");
    window.open(downloadUrl, "_blank");
    setTimeout(() => {
        setDownloadStatus("If nothing downloaded, enable pop-ups and try again.");
    }, 1200);
}

async function handleAutoUpdate() {
    if (autoUpdateInProgress || uploadInProgress) {
        setAutoUpdateStatus("Firmware transfer already in progress.");
        return;
    }

    if (!catalogMatchKey || !catalogMatchEntry) {
        setAutoUpdateStatus("Device has not been matched to a firmware catalog entry yet.");
        return;
    }

    if (typeof JSZip === "undefined") {
        setAutoUpdateStatus("JSZip library failed to load; auto-update unavailable.");
        return;
    }

    const selectedVersion =
        (firmwareDownloadEls.select && firmwareDownloadEls.select.value) || pendingAutoVersion;

    if (!selectedVersion) {
        setAutoUpdateStatus("Select a firmware version first.");
        return;
    }

    const downloadUrl =
        firmwareBaseUrl +
        "?device=" +
        encodeURIComponent(catalogMatchKey) +
        "&version=" +
        encodeURIComponent(selectedVersion);

    try {
        autoUpdateInProgress = true;
        refreshAutoUpdateButtonState();
        setAutoUpdateStatus("Downloading firmware version " + selectedVersion + "…");

        const response = await fetch(downloadUrl, { cache: "no-cache" });
        if (!response.ok) {
            throw new Error("Download failed (" + response.status + ")");
        }

        const arrayBuffer = await response.arrayBuffer();
        const zip = await JSZip.loadAsync(arrayBuffer);
        const binFiles = zip.file(/\.bin$/i) || [];
        const firmwareEntry =
            binFiles.find((file) => /firmware\.bin$/i.test(file.name)) || binFiles[0];

        if (!firmwareEntry) {
            throw new Error("Downloaded package does not contain a firmware .bin file.");
        }

        setAutoUpdateStatus("Preparing firmware image…");
        const firmwareBlob = await firmwareEntry.async("blob");

        if (typeof uploadFirmwareBlob !== "function") {
            throw new Error("Uploader is not ready yet. Please retry in a moment.");
        }

        setAutoUpdateStatus("Uploading firmware to device…");
        await uploadFirmwareBlob(firmwareBlob, firmwareEntry.name || "firmware.bin", true);
        setAutoUpdateStatus("Auto update uploaded. Device should reboot shortly.");
    } catch (err) {
        console.error("Auto update failed", err);
        setAutoUpdateStatus("Auto update failed: " + err.message);
    } finally {
        autoUpdateInProgress = false;
        refreshAutoUpdateButtonState();
    }
}

function connectToServer() {
    connectAttempts += 1;
    const isReconnect = connectAttempts > 1;
    const editorContainer = document.getElementById("editor");
    editorContainer.textContent = "Connecting to server: " + ip + "…";

    const showConfig = true;

    fetch("http://" + ip + "?config=" + (showConfig ? "1" : "0"))
        .then((response) => response.json())
        .then((_data) => {
            data = _data;
            syncSettingsMetadata(data);
            console.log("Config:", data);
            buildStructure();
            initWebSocket();
            if (isReconnect) {
                updateFirmwareStatusUI({ state: "checking", message: "Reconnected. Refreshing firmware status…" });
                fetchDeviceHostInfo();
            }
        })
        .catch((err) => {
            console.error("Error fetching config:", err);
            editorContainer.textContent =
                "Error connecting to server. Retrying…";
            setTimeout(connectToServer, 1500);
        });
}

function initWebSocket() {
    // Create native WebSocket
    oscWS = new WebSocket("ws://" + ip);
    oscWS.binaryType = "arraybuffer";

    oscWS.addEventListener("open", function () {
        console.log("WebSocket opened");
    });

    oscWS.addEventListener("close", function () {
        console.log("WebSocket closed, reconnecting…");
        setTimeout(connectToServer, 800);
    });

    oscWS.addEventListener("error", function (error) {
        console.log("WebSocket Error", error);
    });

    oscWS.addEventListener("message", function (event) {
        // Check if it's a text message (JSON for logs)
        if (typeof event.data === 'string') {
            try {
                const jsonMsg = JSON.parse(event.data);
                if (jsonMsg.COMMAND === 'LOG') {
                    handleLogMessage(jsonMsg.DATA);
                }
            } catch (e) {
                // Not JSON, ignore
            }
            return;
        }

        // Otherwise, expect binary OSC data, decode with osc-mini:
        const oscMsg = OscMini.unpack(event.data);

        const pid = oscMsg.address.substring(1).replace(/\//g, "-");
        const p = document.getElementById(pid);
        const val =
            oscMsg.args && oscMsg.args[0] ? oscMsg.args[0].value : undefined;

        if (p !== undefined && p !== null && val !== undefined) {
            processParameterFeedback(p, val);
        }
    });
}

// ==============================
// Editor generation
// ==============================
function buildStructure() {
    const editorContainer = document.getElementById("editor");
    editorContainer.innerHTML = "";
    generateEditor(data, editorContainer, 0);
}

async function generateEditor(node, parentElement, level) {
    const containerDiv = parentElement.appendChild(
        createContainerEditor(node, node.DESCRIPTION, level)
    );

    for (const key in node.CONTENTS) {
        const item = node.CONTENTS[key];

        if (item.TYPE) {
            const paramsContainer = containerDiv.querySelector(".parameters");
            paramsContainer.appendChild(createParameterEditor(item, level + 1));
        } else if (item.CONTENTS) {
            const componentsContainer = containerDiv.querySelector(".components");
            generateEditor(item, componentsContainer, level + 1);
        }
    }
}

function createContainerEditor(item, key, level) {
    const itemDiv = document.createElement("div");
    itemDiv.classList.add("component", "level" + level);
    itemDiv.innerHTML =
        '<p class="title">' +
        key +
        '</p><div class="parameters"></div><div class="components"></div>';
    return itemDiv;
}

function createParameterEditor(item, level) {
    item.id = item.FULL_PATH.substring(1).replace(/\//g, "-");
    let paramDiv;

    if (item.RANGE != null && item.RANGE[0].VALS != null) {
        paramDiv = createEnumParameterEditor(item, level);
    } else {
        switch (item.TYPE) {
            case "I":
                paramDiv = createTriggerEditor(item, level);
                break;
            case "i":
                paramDiv = createIntParameterEditor(item, level);
                break;
            case "f":
                paramDiv = createFloatParameterEditor(item, level);
                break;
            case "s":
                paramDiv = createStringParameterEditor(item, level);
                break;
            case "b":
            case "T":
            case "F":
                paramDiv = createBoolParameterEditor(item, level);
                break;
            default:
                paramDiv = createDefaultParameterEditor(item, level);
                break;
        }
    }

    if (item.ACCESS === 1) {
        const pMain = paramDiv.querySelector("#" + item.id);
        if (pMain) pMain.disabled = true;
        const pVal = paramDiv.querySelector("#" + item.id + "-value");
        if (pVal) pVal.disabled = true;
    }

    if (item.TAGS !== undefined && item.TAGS.includes("config")) {
        paramDiv.classList.add("config");
        const showConfigCheckbox = document.getElementById("showConfig");
        if (showConfigCheckbox && !showConfigCheckbox.checked) {
            paramDiv.style.display = "none";
        }
    }

    return paramDiv;
}

function createTriggerEditor(item, level) {
    const paramDiv = document.createElement("div");
    paramDiv.classList.add("parameter");
    paramDiv.innerHTML =
        '<input type="button" id="' +
        item.id +
        '" value="' +
        item.DESCRIPTION +
        '" onclick=\'sendTrigger("' +
        item.FULL_PATH +
        '")\' />';
    return paramDiv;
}

function createIntParameterEditor(item, level) {
    const paramDiv = document.createElement("div");
    paramDiv.classList.add("parameter");
    const range =
        item.RANGE !== undefined
            ? 'min="' +
            item.RANGE[0].MIN +
            '" max="' +
            item.RANGE[0].MAX +
            '"'
            : "";
    paramDiv.innerHTML =
        "<label>" +
        item.DESCRIPTION +
        "</label><input type=\"number\" " +
        range +
        ' id="' +
        item.id +
        '" value="' +
        item.VALUE[0] +
        '" oninput=\'sendParameterValue("' +
        item.FULL_PATH +
        '", "i", this.value)\' />';
    return paramDiv;
}

function createFloatParameterEditor(item, level) {
    const paramDiv = document.createElement("div");
    paramDiv.classList.add("parameter");
    const hasRange = item.RANGE != null;
    const range = hasRange
        ? 'min="' +
        item.RANGE[0].MIN +
        '" max="' +
        item.RANGE[0].MAX +
        '"'
        : "";
    const inputType = hasRange ? "range" : "number";
    const rangeText = hasRange
        ? '<input type="text" class="sliderText" id="' +
        item.id +
        '-value" value="' +
        item.VALUE[0].toFixed(3) +
        '">'
        : "";
    const updateRange = hasRange
        ? "this.nextElementSibling.value = parseFloat(this.value).toFixed(3);"
        : "";

    paramDiv.innerHTML =
        "<label>" +
        item.DESCRIPTION +
        "</label><input type=\"" +
        inputType +
        "\" " +
        range +
        ' step="any" id="' +
        item.id +
        '" value="' +
        item.VALUE[0] +
        '" oninput=\'' +
        updateRange +
        'sendParameterValue("' +
        item.FULL_PATH +
        '", "f", this.value)\' />' +
        rangeText;
    return paramDiv;
}

function createStringParameterEditor(item, level) {
    const paramDiv = document.createElement("div");
    paramDiv.classList.add("parameter");
    paramDiv.innerHTML =
        "<label>" +
        item.DESCRIPTION +
        '</label><input type="text" id="' +
        item.id +
        '" value="' +
        item.VALUE[0] +
        '" onchange=\'sendParameterValue("' +
        item.FULL_PATH +
        '", "s", this.value)\' />';
    return paramDiv;
}

function createBoolParameterEditor(item, level) {
    const paramDiv = document.createElement("div");
    paramDiv.classList.add("parameter");
    const checked = item.VALUE[0] ? "checked" : "";
    paramDiv.innerHTML =
        "<label>" +
        item.DESCRIPTION +
        '</label><input type="checkbox" id="' +
        item.id +
        '" ' +
        checked +
        ' oninput=\'sendParameterValue("' +
        item.FULL_PATH +
        '", "i", this.checked)\' />';

    if (item.DESCRIPTION === "enabled") {
        paramDiv.classList.add("enable-param");
    }

    return paramDiv;
}

function createEnumParameterEditor(item, level) {
    const paramDiv = document.createElement("div");
    paramDiv.classList.add("parameter");
    let options = "";
    for (const key in item.RANGE[0].VALS) {
        const value = item.RANGE[0].VALS[key];
        const selected = value === item.VALUE[0] ? "selected" : "";
        options +=
            '<option value="' +
            value +
            '" ' +
            selected +
            ">" +
            value +
            "</option>";
    }
    paramDiv.innerHTML =
        "<label>" +
        item.DESCRIPTION +
        '</label><select id="' +
        item.id +
        '" oninput=\'sendParameterValue("' +
        item.FULL_PATH +
        '", "s", this.value)\'>' +
        options +
        "</select>";
    return paramDiv;
}

function createDefaultParameterEditor(item, level) {
    const paramDiv = document.createElement("div");
    paramDiv.classList.add("parameter");
    paramDiv.innerHTML =
        "<label>" +
        item.DESCRIPTION +
        '</label><input type="text" id="' +
        item.id +
        '" value="' +
        item.VALUE[0] +
        '" oninput=\'sendParameterValue("' +
        item.FULL_PATH +
        '", "f", this.value)\' />';
    return paramDiv;
}

// ==============================
// OSC helpers
// ==============================
function isWsOpen() {
    return oscWS && oscWS.readyState === WebSocket.OPEN;
}

function sendTrigger(fullPath) {
    if (!isWsOpen()) return;
    const msg = { address: fullPath };
    oscWS.send(OscMini.pack(msg));
}

function sendParameterValue(fullPath, type, value) {
    if (!isWsOpen()) return;
    const msg = {
        address: fullPath,
        args: [{ type: type, value: value }]
    };
    oscWS.send(OscMini.pack(msg));
}

function sendCommand(command) {
    if (!isWsOpen()) return;
    const msg = { address: command };
    oscWS.send(OscMini.pack(msg));
}

function restartDevice() {
    sendCommand("/restart");
    setTimeout(() => connectToServer(), 1000);
}

function updateShowConfig() {
    const show = document.getElementById("showConfig").checked;
    const configElems = document.querySelectorAll(".parameter.config");
    configElems.forEach((el) => {
        el.style.display = show ? "" : "none";
    });
}

function processParameterFeedback(p, val) {
    if (p.type === "checkbox") {
        p.checked = !!val;
    } else {
        p.value = val;
    }
    if (p.type === "range" && p.nextElementSibling) {
        p.nextElementSibling.value = parseFloat(p.value).toFixed(3);
    }
}

document
    .getElementById("showConfig")
    .addEventListener("change", updateShowConfig);

// ==============================
// Logger Panel
// ==============================
let logCount = 0;
const maxLogs = 500; // Limit to prevent memory issues
let logFilterValue = "";

function createLogEntryElement({ timestamp, source, message, type }) {
    const logEntry = document.createElement("div");
    logEntry.classList.add("log-entry", type || "info");

    const timeSpan = document.createElement("span");
    timeSpan.classList.add("log-time");
    timeSpan.textContent = timestamp || new Date().toLocaleTimeString();
    logEntry.appendChild(timeSpan);

    const sourceSpan = document.createElement("span");
    sourceSpan.classList.add("log-source");
    sourceSpan.textContent = `[${source || "Unknown"}]`;
    logEntry.appendChild(sourceSpan);

    logEntry.appendChild(createLogMessageSpan(message));

    return logEntry;
}

function createLogMessageSpan(message) {
    const span = document.createElement("span");
    span.classList.add("log-message");

    const normalized = (message ?? "")
        .toString()
        .replace(/\r\n/g, "\n")
        .replace(/\r/g, "\n")
        .replace(/\\n/g, "\n");

    const parts = normalized.split("\n");
    parts.forEach((part, index) => {
        span.appendChild(document.createTextNode(part));
        if (index < parts.length - 1) {
            span.appendChild(document.createElement("br"));
        }
    });

    return span;
}

function handleLogMessage(data) {
    const loggerContent = document.getElementById("loggerContent");
    const logEntry = createLogEntryElement({
        timestamp: new Date().toLocaleTimeString(),
        source: data.source || "Unknown",
        message: data.message || "",
        type: data.type || "info",
    });

    loggerContent.appendChild(logEntry);
    logCount++;
    document.getElementById("logCount").textContent = logCount;

    applyLogFilter();

    if (loggerContent.scrollHeight - loggerContent.scrollTop - loggerContent.clientHeight < 40) {
        loggerContent.scrollTop = loggerContent.scrollHeight;
    }

    if (logCount > maxLogs) {
        const firstLog = loggerContent.querySelector(".log-entry:not(:first-child)");
        if (firstLog) {
            firstLog.remove();
            logCount--;
        }
    }
}

function toggleLogger() {
    const panel = document.getElementById("loggerPanel");
    const isOpen = panel.classList.contains("open");

    if (isOpen) {
        panel.classList.remove("open");
        // Disable sendDebugLogs
        sendParameterValue("/comm/server/sendDebugLogs", "i", 0);
    } else {
        panel.classList.add("open");
        // Enable sendDebugLogs
        sendParameterValue("/comm/server/sendDebugLogs", "i", 1);
    }
}

function clearLogs() {
    const loggerContent = document.getElementById("loggerContent");
    loggerContent.innerHTML = "";
    const systemEntry = createLogEntryElement({
        timestamp: new Date().toLocaleTimeString(),
        source: "System",
        message: "Logs cleared.",
        type: "info",
    });
    loggerContent.appendChild(systemEntry);
    logCount = 1;
    document.getElementById("logCount").textContent = logCount;
    applyLogFilter();
}

document.getElementById("loggerToggle").addEventListener("click", toggleLogger);
document.getElementById("loggerClose").addEventListener("click", toggleLogger);
document.getElementById("clearLogs").addEventListener("click", clearLogs);
const statsButton = document.getElementById("showStatsBtn");
if (statsButton) {
    statsButton.addEventListener("click", () => sendCommand("/root/stats"));
}

const logFilterInput = document.getElementById("logFilter");
if (logFilterInput) {
    logFilterInput.addEventListener("input", (event) => {
        logFilterValue = event.target.value.toLowerCase();
        applyLogFilter();
    });
}

function applyLogFilter() {
    const loggerContent = document.getElementById("loggerContent");
    if (!loggerContent) return;
    const entries = loggerContent.querySelectorAll(".log-entry");
    entries.forEach((entry) => {
        if (!logFilterValue) {
            entry.style.display = "";
            return;
        }
        const text = entry.textContent || "";
        entry.style.display = text.toLowerCase().includes(logFilterValue) ? "" : "none";
    });
}

// ==============================
// OTA upload logic (no jQuery)
// ==============================
(function setupOtaUploader() {
    const fileInput = document.getElementById("firmware");
    const fileMeta = document.getElementById("fileMeta");
    const uploadBtn = document.getElementById("uploadBtn");
    const uploadBtnLabel = document.getElementById("uploadBtnLabel");
    const uploadSpinner = document.getElementById("uploadSpinner");
    const progressFill = document.getElementById("progressFill");
    const progressLabel = document.getElementById("progressLabel");
    const progressValue = document.getElementById("progressValue");
    const statusText = document.getElementById("statusText");
    const defaultUploadLabel = uploadBtnLabel.textContent;

    let selectedFile = null;

    function formatBytes(bytes) {
        if (bytes === undefined || bytes === null) return "–";
        const units = ["B", "KB", "MB", "GB"];
        let i = 0;
        let value = bytes;
        while (value >= 1024 && i < units.length - 1) {
            value /= 1024;
            i++;
        }
        return value.toFixed(i === 0 ? 0 : 1) + " " + units[i];
    }

    function setStatus(mode, message) {
        statusText.classList.remove("ok", "error", "warn");
        if (mode) statusText.classList.add(mode);
        statusText.innerHTML =
            "<strong>Status:</strong> " + (message || "idle");
    }

    function setProgress(percent, label) {
        const clamped = Math.max(0, Math.min(100, percent || 0));
        progressFill.style.transform = "scaleX(" + clamped / 100 + ")";
        progressValue.textContent = clamped.toFixed(0) + "%";
        if (label) progressLabel.textContent = label;
    }

    function refreshUploadButtonState() {
        uploadBtn.disabled = uploadInProgress || !selectedFile;
    }

    fileInput.addEventListener("change", () => {
        const file = fileInput.files[0];
        selectedFile = file || null;

        if (!selectedFile) {
            fileMeta.innerHTML =
                '<span class="file-pill muted">No file selected</span>';
            setStatus("", "idle");
            setProgress(0, "Waiting for upload…");
            refreshUploadButtonState();
            return;
        }

        const tooBig = selectedFile.size > 3 * 1024 * 1024; // soft warning

        fileMeta.innerHTML =
            '<span class="file-pill">' +
            selectedFile.name +
            "</span>" +
            '<span class="file-pill ' +
            (tooBig ? "warn" : "") +
            '">' +
            formatBytes(selectedFile.size) +
            "</span>";

        setStatus(
            tooBig ? "warn" : "",
            tooBig
                ? "Large file — make sure it matches your partition size."
                : "Ready to upload."
        );
        setProgress(0, "Ready.");
        refreshUploadButtonState();
    });

    async function handleManualUpload() {
        if (!selectedFile || uploadInProgress) return;
        try {
            await performFirmwareUpload(selectedFile, selectedFile.name, false);
        } catch (err) {
            console.warn("Manual upload failed", err);
        }
    }

    uploadBtn.addEventListener("click", handleManualUpload);

    function performFirmwareUpload(fileBlob, filename, fromAuto) {
        return new Promise((resolve, reject) => {
            if (!fileBlob) {
                reject(new Error("No firmware selected."));
                return;
            }

            const finalFilename = "firmware.bin";

            uploadInProgress = true;
            uploadSpinner.style.display = "inline-block";
            uploadBtnLabel.textContent = fromAuto ? "Auto updating…" : "Uploading…";
            setStatus(
                "",
                fromAuto
                    ? "Auto update in progress… do not power off."
                    : "Uploading firmware… do not power off."
            );
            setProgress(fromAuto ? 5 : 2, fromAuto ? "Uploading auto update…" : "Starting upload…");
            refreshUploadButtonState();
            refreshAutoUpdateButtonState();

            const xhr = new XMLHttpRequest();
            xhr.open("POST", "http://" + ip + "/uploadFile", true);

            xhr.upload.onprogress = function (evt) {
                if (evt.lengthComputable) {
                    const percent = (evt.loaded / evt.total) * 100;
                    setProgress(percent, fromAuto ? "Auto uploading…" : "Uploading…");
                } else {
                    setProgress(20, fromAuto ? "Auto uploading…" : "Uploading…");
                }
            };

            xhr.onload = function () {
                const ok = xhr.status >= 200 && xhr.status < 300;
                uploadInProgress = false;
                uploadSpinner.style.display = "none";
                refreshUploadButtonState();
                refreshAutoUpdateButtonState();

                if (ok) {
                    setProgress(100, "Upload complete. Rebooting…");
                    setStatus(
                        "ok",
                        fromAuto
                            ? "Auto update uploaded — device should reboot shortly."
                            : "Upload successful — device should reboot into the new firmware."
                    );
                    if (fromAuto) {
                        uploadBtnLabel.textContent = defaultUploadLabel;
                    } else {
                        uploadBtnLabel.textContent = "Uploaded";
                    }
                    resolve();
                } else {
                    setProgress(0, "Upload failed.");
                    setStatus("error", "Upload failed with status " + xhr.status + ".");
                    uploadBtnLabel.textContent = fromAuto ? defaultUploadLabel : "Upload failed";
                    reject(new Error("Upload failed with status " + xhr.status));
                }
            };

            xhr.onerror = function () {
                uploadInProgress = false;
                uploadSpinner.style.display = "none";
                setProgress(0, "Network error.");
                setStatus("error", "Network error during upload.");
                uploadBtnLabel.textContent = fromAuto ? defaultUploadLabel : "Upload firmware";
                refreshUploadButtonState();
                refreshAutoUpdateButtonState();
                reject(new Error("Network error during upload."));
            };

            const formData = new FormData();
            const fileForJuiceStyle = new File(
                [fileBlob],
                finalFilename,
                { type: "text/plain" }
            );

            // Field name must match the JUCE uploader handler; do not override Content-Type headers manually.
            formData.append("uploadData", fileForJuiceStyle, finalFilename);
            xhr.send(formData);
        });
    }

    uploadFirmwareBlob = function (blob, filename, fromAuto = false) {
        return performFirmwareUpload(blob, filename, fromAuto);
    };
})();