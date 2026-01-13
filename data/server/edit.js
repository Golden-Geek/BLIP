// ==============================
// Connection & configuration
// ==============================
const searchParams = new URLSearchParams(window.location.search);
const urlIp = searchParams.get("ip");
//local should detect any ip like xxx.xxx.xxx.xxx or any .local domain

const local =  window.location.hostname.includes(".local") || /^[0-9]{1,3}(\.[0-9]{1,3}){3}$/.test(window.location.hostname);
// Extract port if present in the URL (e.g., ?ip=192.168.1.10:8888 or from window.location)
let ip = urlIp;
if (!ip) {
    ip = window.location.hostname;
    if (window.location.port && window.location.port !== "80" && window.location.port !== "443") {
        ip += ":" + window.location.port;
    }
    if (!local && !window.location.port) {
        ip = "192.168.1.193";
    }
}
let data = {};
let oscWS;
let connectAttempts = 0;
let forcedDisconnectMessage = null;

const firmwareBaseUrl = "https://www.goldengeek.org/blip/download/firmware/getFirmwares.php";
const firmwareListUrl = firmwareBaseUrl + "?list";
const FIRMWARE_BLEEDING_MANIFEST_URL = "https://www.goldengeek.org/blip/download/firmware/bleeding-edge.php";
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

const connectionBadgeEl = document.getElementById("connectionBadge");

const SERVER_BASE_URL = "https://www.goldengeek.org/blip/download/server/";
const SERVER_MANIFEST_URL = SERVER_BASE_URL + "latest.php";
const SERVER_DOWNLOAD_BASE = SERVER_BASE_URL + "servers/";
const SERVER_BLEEDING_MANIFEST_URL = SERVER_BASE_URL + "latest.php?file=bleeding-edge.json";

const serverVersionEls = {
    card: document.getElementById("serverVersionCard"),
    local: document.getElementById("serverVersionLocal"),
    remote: document.getElementById("serverVersionRemote"),
    status: document.getElementById("serverVersionStatus"),
    action: document.getElementById("serverVersionAction"),
    upload: document.getElementById("serverUploadAction"),
    uploadStatus: document.getElementById("serverUploadStatus"),
};

const bleedingEdgeEls = {
    panel: document.getElementById("bleedingEdgePanel"),
    status: document.getElementById("bleedingEdgeStatus"),
    action: document.getElementById("bleedingEdgeAction"),
};

const firmwareBleedingEls = {
    panel: document.getElementById("firmwareBleedingPanel"),
    status: document.getElementById("firmwareBleedingStatus"),
    action: document.getElementById("firmwareBleedingAction"),
};

let localServerVersion = null;
let latestServerManifest = null;
let serverVersionFetchPromise = null;
let serverUploadInProgress = false;
let bleedingEdgeManifest = null;
let bleedingEdgeDownloadUrl = null;
let firmwareBleedingManifest = null;
let firmwareBleedingDownloadUrl = null;
let firmwareBleedingFetchPromise = null;
let firmwareBleedingPendingKey = null;

const firmwarePanelEls = {
    card: document.getElementById("firmwareCard"),
    body: document.getElementById("firmwareBody"),
    toggle: document.getElementById("firmwareToggle"),
    label: document.getElementById("firmwareToggleLabel"),
};

let firmwarePanelCollapsed = true;
if (firmwarePanelEls.card) {
    firmwarePanelCollapsed = firmwarePanelEls.card.dataset.collapsed !== "false";
}

const baseDocumentTitle = document.title || "BLIP Control Panel";

function isBleedingEdgeVersion(value) {
    return typeof value === "string" && value.toLowerCase() === "bleeding-edge";
}

function coerceReleaseManifest(manifest) {
    if (!manifest || typeof manifest !== "object") {
        return null;
    }
    if (isBleedingEdgeVersion(manifest.version)) {
        return null;
    }
    if (!manifest.url && !manifest.version) {
        return null;
    }
    return manifest;
}

setConnectionStatus("connecting");

updateDeviceMetaDisplay();
initServerVersionPanel();
initBleedingEdgePanel();
initFirmwareBleedingPanel();
initFirmwarePanelToggle();

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

function initServerVersionPanel() {
    if (!serverVersionEls.card) {
        return;
    }

    localServerVersion = readLocalServerVersionComment();
    if (serverVersionEls.local) {
        serverVersionEls.local.textContent = localServerVersion || "—";
    }

    if (serverVersionEls.status) {
        serverVersionEls.status.textContent = "Checking online dashboard bundle…";
    }

    if (serverVersionEls.uploadStatus) {
        serverVersionEls.uploadStatus.textContent = "Upload not started.";
    }

    if (serverVersionEls.action) {
        serverVersionEls.action.addEventListener("click", () => {
            if (!latestServerManifest) return;
            const directUrl = latestServerManifest.url;
            const inferredUrl = latestServerManifest.version
                ? SERVER_DOWNLOAD_BASE + "server-compressed-" + latestServerManifest.version + ".zip"
                : null;
            const targetUrl = directUrl || inferredUrl;
            if (targetUrl) {
                window.open(targetUrl, "_blank");
            }
        });
    }

    if (serverVersionEls.upload) {
        serverVersionEls.upload.addEventListener("click", handleServerBundleUpload);
    }

    fetchServerManifestVersion().catch(() => { });
}

function initBleedingEdgePanel() {
    if (!bleedingEdgeEls.panel) {
        return;
    }

    setBleedingEdgeStatus("Checking master branch dashboard build…", true);

    if (bleedingEdgeEls.action) {
        bleedingEdgeEls.action.disabled = true;
        bleedingEdgeEls.action.addEventListener("click", () => {
            if (bleedingEdgeDownloadUrl) {
                window.open(bleedingEdgeDownloadUrl, "_blank");
            }
        });
    }

    fetchBleedingEdgeManifest().catch(() => { });
}

function setConnectionStatus(state, message) {
    if (!connectionBadgeEl) {
        return;
    }
    const labels = {
        connecting: "Connecting…",
        online: "Online",
        offline: "Offline",
    };
    connectionBadgeEl.dataset.state = state || "connecting";
    connectionBadgeEl.textContent = message || labels[state] || labels.connecting;
}

function readLocalServerVersionComment() {
    const nodes = document.childNodes || [];
    for (let i = 0; i < nodes.length; i++) {
        const node = nodes[i];
        if (node && node.nodeType === Node.COMMENT_NODE) {
            const match = /Version\s+([^\s]+)/i.exec(node.nodeValue || "");
            if (match && match[1]) {
                return match[1].trim();
            }
        }
    }
    return null;
}

function fetchServerManifestVersion() {
    if (!serverVersionEls.card) {
        return Promise.resolve(null);
    }

    if (serverVersionFetchPromise) {
        return serverVersionFetchPromise;
    }

    serverVersionEls.card.dataset.state = "checking";
    if (serverVersionEls.remote) {
        serverVersionEls.remote.textContent = "Checking…";
    }

    serverVersionFetchPromise = fetch(SERVER_MANIFEST_URL, { cache: "no-store" })
        .then((resp) => {
            if (!resp.ok) {
                throw new Error("Manifest request failed with status " + resp.status);
            }
            return resp.json();
        })
        .then((manifest) => {
            latestServerManifest = coerceReleaseManifest(manifest);
            const remoteVersion = latestServerManifest && latestServerManifest.version ? latestServerManifest.version : null;
            if (serverVersionEls.remote) {
                serverVersionEls.remote.textContent = remoteVersion || "—";
            }
            updateServerVersionUIState(remoteVersion);
            return latestServerManifest;
        })
        .catch((err) => {
            console.warn("Failed to fetch server bundle manifest", err);
            if (serverVersionEls.status) {
                serverVersionEls.status.textContent = "Unable to reach the dashboard update server.";
            }
            if (serverVersionEls.card) {
                serverVersionEls.card.dataset.state = "error";
            }
            if (serverVersionEls.action) {
                serverVersionEls.action.disabled = true;
            }
            if (serverVersionEls.upload && !serverUploadInProgress) {
                serverVersionEls.upload.disabled = true;
            }
            throw err;
        })
        .finally(() => {
            serverVersionFetchPromise = null;
        });

    return serverVersionFetchPromise;
}

function setBleedingEdgeStatus(message, disableAction) {
    if (!bleedingEdgeEls.panel) {
        return;
    }
    if (bleedingEdgeEls.status) {
        bleedingEdgeEls.status.textContent = message || "";
    }
    if (bleedingEdgeEls.action) {
        if (typeof disableAction === "boolean") {
            bleedingEdgeEls.action.disabled = disableAction;
        } else if (bleedingEdgeDownloadUrl) {
            bleedingEdgeEls.action.disabled = false;
        }
    }
}

function fetchBleedingEdgeManifest() {
    if (!bleedingEdgeEls.panel) {
        return Promise.resolve(null);
    }

    return fetch(SERVER_BLEEDING_MANIFEST_URL, { cache: "no-store" })
        .then((resp) => {
            if (!resp.ok) {
                throw new Error("Bleeding edge manifest request failed with status " + resp.status);
            }
            return resp.json();
        })
        .then((manifest) => {
            bleedingEdgeManifest = manifest || {};
            updateBleedingEdgeUI(bleedingEdgeManifest);
            return bleedingEdgeManifest;
        })
        .catch((err) => {
            console.warn("Failed to fetch bleeding edge dashboard manifest", err);
            setBleedingEdgeStatus("Unable to load bleeding edge bundle.", true);
            throw err;
        });
}

function updateBleedingEdgeUI(manifest) {
    if (!bleedingEdgeEls.panel) {
        return;
    }

    const downloadUrl = getServerBundleDownloadUrl(manifest);
    bleedingEdgeDownloadUrl = downloadUrl;

    let status = "Latest master build ready.";
    if (manifest && manifest.generatedAt) {
        try {
            const timestamp = new Date(manifest.generatedAt);
            status = `Latest master build • ${timestamp.toLocaleString()}`;
        } catch (err) {
            console.warn("Failed to parse bleeding edge timestamp", err);
        }
    } else if (manifest && manifest.version) {
        status = `Latest master build (${manifest.version})`;
    }

    setBleedingEdgeStatus(status, !downloadUrl);
}

function initFirmwareBleedingPanel() {
    if (!firmwareBleedingEls.panel) {
        return;
    }

    setFirmwareBleedingStatus(
        "Waiting for device match before checking bleeding edge firmware.",
        true
    );

    if (firmwareBleedingEls.action) {
        firmwareBleedingEls.action.disabled = true;
        firmwareBleedingEls.action.addEventListener("click", () => {
            if (firmwareBleedingDownloadUrl) {
                window.open(firmwareBleedingDownloadUrl, "_blank");
            }
        });
    }

    fetchFirmwareBleedingManifest().catch(() => { });
}

function setFirmwareBleedingStatus(message, disableAction) {
    if (!firmwareBleedingEls.panel) {
        return;
    }
    if (firmwareBleedingEls.status) {
        firmwareBleedingEls.status.textContent = message || "";
    }
    if (firmwareBleedingEls.action) {
        if (typeof disableAction === "boolean") {
            firmwareBleedingEls.action.disabled = disableAction;
        } else {
            firmwareBleedingEls.action.disabled = !firmwareBleedingDownloadUrl;
        }
    }
}

function fetchFirmwareBleedingManifest() {
    if (!firmwareBleedingEls.panel) {
        return Promise.resolve(null);
    }

    if (firmwareBleedingFetchPromise) {
        return firmwareBleedingFetchPromise;
    }

    firmwareBleedingFetchPromise = fetch(FIRMWARE_BLEEDING_MANIFEST_URL, { cache: "no-store" })
        .then((resp) => {
            if (!resp.ok) {
                throw new Error("Bleeding edge firmware manifest request failed with status " + resp.status);
            }
            return resp.json();
        })
        .then((manifest) => {
            firmwareBleedingManifest = manifest || {};
            if (firmwareBleedingPendingKey) {
                updateFirmwareBleedingPanelMatch(firmwareBleedingPendingKey);
            }
            return firmwareBleedingManifest;
        })
        .catch((err) => {
            console.warn("Failed to fetch bleeding edge firmware manifest", err);
            setFirmwareBleedingStatus("Unable to load bleeding edge firmware info.", true);
            throw err;
        })
        .finally(() => {
            firmwareBleedingFetchPromise = null;
        });

    return firmwareBleedingFetchPromise;
}

function updateFirmwareBleedingPanelMatch(matchKey, catalogEntry) {
    if (!firmwareBleedingEls.panel) {
        return;
    }

    firmwareBleedingPendingKey = matchKey || null;
    firmwareBleedingDownloadUrl = null;

    if (!matchKey) {
        setFirmwareBleedingStatus(
            "Bleeding edge firmware downloads unlock once this device is matched to the catalog.",
            true
        );
        return;
    }

    if (!firmwareBleedingManifest) {
        setFirmwareBleedingStatus("Checking bleeding edge firmware for this device…", true);
        fetchFirmwareBleedingManifest().catch(() => { });
        return;
    }

    const entryMatch = findFirmwareBleedingEntry(matchKey);
    if (!entryMatch) {
        const readableName = (catalogEntry && (catalogEntry.name || catalogEntry.displayName)) || matchKey;
        setFirmwareBleedingStatus("No bleeding edge firmware published for " + readableName + " yet.", true);
        return;
    }

    const downloadUrl = buildFirmwareBleedingDownloadUrl(entryMatch.key, entryMatch.entry);
    if (!downloadUrl) {
        setFirmwareBleedingStatus("Bleeding edge firmware entry missing download URL.", true);
        return;
    }

    firmwareBleedingDownloadUrl = downloadUrl;

    let status = entryMatch.entry && entryMatch.entry.version
        ? "Version " + entryMatch.entry.version + " ready."
        : "Latest master build ready.";
    const timestampLabel = formatBleedingTimestamp(entryMatch.entry && entryMatch.entry.generatedAt);
    if (timestampLabel) {
        status += " • " + timestampLabel;
    }

    setFirmwareBleedingStatus(status, false);
}

function findFirmwareBleedingEntry(deviceKey) {
    if (!firmwareBleedingManifest) {
        return null;
    }
    const normalizedKey = normalizeName(deviceKey);
    const collection =
        (firmwareBleedingManifest.devices && typeof firmwareBleedingManifest.devices === "object")
            ? firmwareBleedingManifest.devices
            : firmwareBleedingManifest;

    if (!collection || typeof collection !== "object") {
        return null;
    }

    let match = null;
    Object.keys(collection).some((key) => {
        const entry = collection[key];
        const entryKey = entry && (entry.key || entry.device || entry.name);
        if (normalizeName(key) === normalizedKey || normalizeName(entryKey) === normalizedKey) {
            match = { key, entry };
            return true;
        }
        return false;
    });
    return match;
}

function buildFirmwareBleedingDownloadUrl(deviceKey, entry) {
    if (entry && entry.url) {
        return entry.url;
    }
    if (!deviceKey) {
        return null;
    }
    const version = (entry && entry.version) || "bleeding-edge";
    return (
        firmwareBaseUrl +
        "?device=" +
        encodeURIComponent(deviceKey) +
        "&version=" +
        encodeURIComponent(version)
    );
}

function formatBleedingTimestamp(value) {
    if (!value) {
        return null;
    }
    try {
        const timestamp = new Date(value);
        if (Number.isNaN(timestamp.getTime())) {
            return null;
        }
        return timestamp.toLocaleString();
    } catch (err) {
        console.warn("Failed to parse firmware bleeding edge timestamp", err);
        return null;
    }
}

function updateServerVersionUIState(remoteVersion) {
    if (!serverVersionEls.card) {
        return;
    }

    const hasRemote = remoteVersion && remoteVersion !== "—";
    const hasLocal = !!localServerVersion;
    let state = "checking";
    let message = "";

    if (!hasRemote) {
        state = "error";
        message = "Latest dashboard bundle unavailable.";
        setServerBundleActionEnabled(false);
    } else if (!hasLocal) {
        state = "warn";
        message = "Local version unknown; download latest build to stay in sync.";
        setServerBundleActionEnabled(true);
    } else {
        const comparison = compareVersions(localServerVersion, remoteVersion);
        if (comparison >= 0) {
            state = "ok";
            message = "Dashboard is up to date.";
            setServerBundleActionEnabled(true);
        } else {
            state = "warn";
            message = "New dashboard bundle available.";
            setServerBundleActionEnabled(true);
        }
    }

    serverVersionEls.card.dataset.state = state;
    if (serverVersionEls.status) {
        serverVersionEls.status.textContent = message;
    }
}

function setServerBundleActionEnabled(enabled) {
    if (serverVersionEls.action) {
        serverVersionEls.action.disabled = !enabled;
    }
    if (serverVersionEls.upload) {
        serverVersionEls.upload.disabled = !enabled || serverUploadInProgress;
    }
}

function setServerUploadStatus(message) {
    if (serverVersionEls.uploadStatus) {
        serverVersionEls.uploadStatus.textContent = message || "";
    }
}

function setServerUploadButtonState(label, forceDisable) {
    if (!serverVersionEls.upload) {
        return;
    }
    if (label) {
        serverVersionEls.upload.textContent = label;
    }
    if (typeof forceDisable === "boolean") {
        serverVersionEls.upload.disabled = forceDisable;
    }
}

function hasAvailableServerBundle() {
    return !!(latestServerManifest && (latestServerManifest.url || latestServerManifest.version));
}

function getServerBundleDownloadUrl(manifest) {
    if (!manifest) {
        return null;
    }
    if (manifest.url) {
        return manifest.url;
    }
    if (manifest.version) {
        return SERVER_DOWNLOAD_BASE + "server-compressed-" + manifest.version + ".zip";
    }
    return null;
}

function resolveServerUploadTarget(entryName) {
    let normalized = (entryName || "")
        .replace(/\\\\/g, "/")
        .replace(/^\.\/+/, "")
        .replace(/^\/+/, "");

    const parts = normalized.split("/").filter(Boolean);
    const filename = parts.pop();
    let folder = parts.length ? parts.join("/") : "";

    if (!filename) {
        throw new Error("Bundle entry missing filename.");
    }

    if (!folder) {
        folder = "server";
    }

    if (folder.endsWith("/")) {
        folder = folder.slice(0, -1);
    }

    if (!folder.startsWith("/")) {
        folder = "/" + folder;
    }

    return { folder, filename };
}

async function uploadServerAsset(blob, filename, folder) {
    const targetUrl = new URL("http://" + ip + "/uploadFile");
    if (folder) {
        targetUrl.searchParams.set("folder", folder);
    }

    const formData = new FormData();
    formData.append("uploadData", blob, filename || "file.bin");

    const response = await fetch(targetUrl.toString(), {
        method: "POST",
        body: formData,
    });

    if (!response.ok) {
        let errorText = "";
        try {
            errorText = await response.text();
        } catch (err) {
            console.warn("Unable to read upload error response", err);
        }
        throw new Error(
            "Upload failed (" +
            response.status +
            ")" +
            (errorText ? ": " + errorText.trim() : "")
        );
    }
}

async function handleServerBundleUpload() {
    if (serverUploadInProgress) {
        setServerUploadStatus("Upload already in progress…");
        return;
    }

    if (typeof JSZip === "undefined") {
        setServerUploadStatus("JSZip failed to load; refresh the page and try again.");
        return;
    }

    serverUploadInProgress = true;
    setServerUploadButtonState("Preparing…", true);
    setServerBundleActionEnabled(false);

    try {
        setServerUploadStatus("Fetching latest dashboard manifest…");
        const manifest = (await fetchServerManifestVersion().catch(() => null)) || latestServerManifest;
        if (!manifest) {
            throw new Error("Unable to fetch dashboard manifest.");
        }

        const downloadUrl = getServerBundleDownloadUrl(manifest);
        if (!downloadUrl) {
            throw new Error("Dashboard download URL is missing.");
        }

        const versionLabel = manifest.version ? " " + manifest.version : "";
        setServerUploadStatus("Downloading dashboard bundle" + versionLabel + "…");
        const response = await fetch(downloadUrl, { cache: "no-cache" });
        if (!response.ok) {
            throw new Error("Download failed (" + response.status + ")");
        }

        const zip = await JSZip.loadAsync(await response.arrayBuffer());
        const entries = Object.keys(zip.files)
            .map((key) => zip.files[key])
            .filter((file) => file && !file.dir);

        if (!entries.length) {
            throw new Error("Downloaded bundle did not contain any files.");
        }

        let completed = 0;
        for (const entry of entries) {
            completed += 1;
            const progressLabel = `Uploading ${completed}/${entries.length}…`;
            setServerUploadButtonState(progressLabel, true);
            setServerUploadStatus(`${progressLabel} (${entry.name})`);

            const blob = await entry.async("blob");
            const target = resolveServerUploadTarget(entry.name);
            await uploadServerAsset(blob, target.filename, target.folder);
        }

        setServerUploadStatus(
            "Upload complete. Reload this page to serve version " +
            (manifest.version || localServerVersion || "the latest build") +
            "."
        );

        if (manifest.version) {
            localServerVersion = manifest.version;
            if (serverVersionEls.local) {
                serverVersionEls.local.textContent = localServerVersion;
            }
            updateServerVersionUIState(manifest.version);
        }
    } catch (err) {
        console.error("Server bundle upload failed", err);
        setServerUploadStatus("Upload failed: " + (err && err.message ? err.message : err));
        if (serverVersionEls.card) {
            serverVersionEls.card.dataset.state = "error";
        }
    } finally {
        serverUploadInProgress = false;
        setServerUploadButtonState("Upload to device", !hasAvailableServerBundle());
        setServerBundleActionEnabled(hasAvailableServerBundle());
        if (hasAvailableServerBundle()) {
            updateServerVersionUIState(latestServerManifest.version || "—");
        }
    }
}

function startVersionChecks() {
    if (versionCheckStarted) return;
    versionCheckStarted = true;
    setDownloadStatus("Matching device with firmware catalog…");
    setAutoUpdateAvailability(null, "Matching device with firmware catalog…");
    fetchDeviceHostInfo();
    fetchFirmwareCatalog();
}

function initFirmwarePanelToggle() {
    if (!firmwarePanelEls.card) {
        return;
    }
    setFirmwarePanelCollapsed(firmwarePanelCollapsed, false);
    if (firmwarePanelEls.toggle) {
        firmwarePanelEls.toggle.addEventListener("click", () => {
            setFirmwarePanelCollapsed(!firmwarePanelCollapsed, true);
        });
    }
}

function setFirmwarePanelCollapsed(collapsed, userInitiated) {
    if (!firmwarePanelEls.card) {
        return;
    }
    firmwarePanelCollapsed = collapsed;
    firmwarePanelEls.card.dataset.collapsed = collapsed ? "true" : "false";
    if (firmwarePanelEls.toggle) {
        firmwarePanelEls.toggle.setAttribute("aria-expanded", (!collapsed).toString());
    }
    if (firmwarePanelEls.label) {
        firmwarePanelEls.label.textContent = collapsed ? "Show panel" : "Hide panel";
    }
    if (userInitiated && !collapsed) {
        firmwarePanelEls.card.scrollIntoView({ behavior: "smooth", block: "start" });
    }
}

function ensureFirmwarePanelExpandedForUpdate() {
    if (!firmwarePanelEls.card || !firmwarePanelCollapsed) {
        return;
    }
    setFirmwarePanelCollapsed(false, false);
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

function updateDocumentTitle() {
    const nameValue =
        (deviceInfo && (deviceInfo.NAME || deviceInfo.DEVICE_NAME)) ||
        settingsDeviceName ||
        "";
    const propValueRaw =
        settingsPropId !== null && settingsPropId !== undefined && settingsPropId !== ""
            ? settingsPropId
            : deviceInfo && deviceInfo.PROP_ID;
    const propValue =
        propValueRaw === null || propValueRaw === undefined || propValueRaw === ""
            ? null
            : propValueRaw;

    const descriptorParts = [];
    if (nameValue) {
        descriptorParts.push(nameValue);
    }
    if (propValue !== null) {
        descriptorParts.push("Prop " + propValue);
    }

    document.title = descriptorParts.length
        ? baseDocumentTitle + " — " + descriptorParts.join(" • ")
        : baseDocumentTitle;
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

    updateDocumentTitle();
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
        updateFirmwareBleedingPanelMatch(null, null);
        return;
    }

    const currentVersion = deviceInfo.VERSION || "—";
    updateFirmwareStatusUI({ current: currentVersion });

    if (!firmwareCatalog) {
        updateFirmwareStatusUI({ state: "checking", message: "Waiting for firmware catalog…" });
        setDownloadStatus("Waiting for firmware catalog to load…");
        setAutoUpdateAvailability(null, "Waiting for firmware catalog to load…");
        updateFirmwareBleedingPanelMatch(null, null);
        return;
    }

    const catalogDeviceType = getCatalogDeviceType();
    if (!catalogDeviceType) {
        updateFirmwareStatusUI({ state: "checking", message: "Waiting for device type information…" });
        setDownloadStatus("Waiting for device type info before enabling downloads.");
        setAutoUpdateAvailability(null, "Waiting for device type info before enabling downloads.");
        updateFirmwareBleedingPanelMatch(null, null);
        return;
    }

    const match = findCatalogEntry(catalogDeviceType);
    const catalogEntry = match ? match.entry : null;
    const stableVersions = catalogEntry && Array.isArray(catalogEntry.versions)
        ? catalogEntry.versions.filter((version) => !isBleedingEdgeVersion(version))
        : [];

    if (!catalogEntry || stableVersions.length === 0) {
        updateFirmwareStatusUI({ latest: "—", state: "error", message: "No release firmware found for this device." });
        updateDownloadControls(null, null);
        setDownloadStatus("Could not match this device to any downloadable firmware.");
        setAutoUpdateAvailability(null, "Could not match this device to any downloadable firmware.");
        updateFirmwareBleedingPanelMatch(null, null);
        return;
    }

    const matchKey = match ? match.key : null;
    updateDownloadControls({ ...catalogEntry, versions: stableVersions }, matchKey);
    updateFirmwareBleedingPanelMatch(matchKey, catalogEntry);

    const latestVersion = stableVersions[0];
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
        ensureFirmwarePanelExpandedForUpdate();
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

    const versions = entry && Array.isArray(entry.versions)
        ? entry.versions.filter((version) => !isBleedingEdgeVersion(version))
        : [];

    if (versions.length === 0) {
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

    versions.forEach((version) => {
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
    setConnectionStatus("connecting", isReconnect ? "Reconnecting…" : null);

    const showConfig = true;

    fetch("http://" + ip + "?config=" + (showConfig ? "1" : "0"))
        .then((response) => response.json())
        .then((_data) => {
            data = _data;
            syncSettingsMetadata(data);
            console.log("Config:", data);
            buildStructure();
            initWebSocket();
            setConnectionStatus("online");
            if (isReconnect) {
                updateFirmwareStatusUI({ state: "checking", message: "Reconnected. Refreshing firmware status…" });
                fetchDeviceHostInfo();
            }
        })
        .catch((err) => {
            console.error("Error fetching config:", err);
            editorContainer.textContent =
                "Error connecting to server. Retrying…";
            setConnectionStatus("offline", "Offline — retrying…");
            setTimeout(connectToServer, 1500);
        });
}

function initWebSocket() {
    // Create native WebSocket
    oscWS = new WebSocket("ws://" + ip);
    oscWS.binaryType = "arraybuffer";

    oscWS.addEventListener("open", function () {
        console.log("WebSocket opened");
        setConnectionStatus("online");
    });

    oscWS.addEventListener("close", function () {
        console.log("WebSocket closed, reconnecting…");
        const message = forcedDisconnectMessage || "Connection lost — reconnecting…";
        forcedDisconnectMessage = null;
        setConnectionStatus("offline", message);
        setTimeout(connectToServer, 800);
    });

    oscWS.addEventListener("error", function (error) {
        console.log("WebSocket Error", error);
        setConnectionStatus("offline", "Connection error");
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

        if (oscMsg.address === "/bye") {
            handleDeviceByeMessage(oscMsg);
            return;
        }

        const pid = oscMsg.address.substring(1).replace(/\//g, "-");
        const p = document.getElementById(pid);
        const val =
            oscMsg.args && oscMsg.args[0] ? oscMsg.args[0].value : undefined;

        if (p !== undefined && p !== null && val !== undefined) {
            processParameterFeedback(p, val);
        }
    });
}

function handleDeviceByeMessage(oscMsg) {
    const reason =
        oscMsg.args && oscMsg.args.length > 0 && typeof oscMsg.args[0].value === "string"
            ? oscMsg.args[0].value
            : "unknown";

    const reasonMessages = {
        restart: "Device restarting — reconnecting soon…",
        shutdown: "Device shutting down — connection closed.",
        standby: "Device entering standby — connection closed.",
    };

    const message = reasonMessages[reason] || "Device closed the connection.";

    forcedDisconnectMessage = message;
    setConnectionStatus("offline", message);

    const editorContainer = document.getElementById("editor");
    if (editorContainer) {
        editorContainer.textContent = message;
    }

    if (oscWS && (oscWS.readyState === WebSocket.OPEN || oscWS.readyState === WebSocket.CONNECTING)) {
        try {
            // Force the socket closed so the reconnect logic can kick in immediately.
            oscWS.close(4000, "Device requested /bye");
        } catch (err) {
            console.warn("Failed to close WebSocket after /bye", err);
        }
    }
}

// ==============================
// Editor generation
// ==============================
function buildStructure() {
    const editorContainer = document.getElementById("editor");
    editorContainer.innerHTML = "";
    generateEditor(data, editorContainer, 0);
}

function isEnabledParameter(item) {
    return (
        typeof item.DESCRIPTION === "string" &&
        item.DESCRIPTION.trim().toLowerCase() === "enabled"
    );
}

async function generateEditor(node, parentElement, level) {
    const containerDiv = parentElement.appendChild(
        createContainerEditor(node, node.DESCRIPTION, level)
    );
    const paramsContainer = containerDiv.querySelector(".parameters");
    const toggleSlot = containerDiv.querySelector(".component-toggle-slot");

    for (const key in node.CONTENTS) {
        const item = node.CONTENTS[key];

        if (item.TYPE) {
            const paramEl = createParameterEditor(item, level + 1);
            if (!paramEl) continue;

            if (isEnabledParameter(item) && toggleSlot) {
                toggleSlot.appendChild(paramEl);
            } else {
                paramsContainer.appendChild(paramEl);
            }
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
        '<div class="component-header">' +
        '  <div class="component-toggle-slot"></div>' +
        '  <p class="title">' +
        key +
        "  </p>" +
        "</div>" +
        '<div class="parameters"></div><div class="components"></div>';
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
    const isEnabled = isEnabledParameter(item);

    if (isEnabled) {
        paramDiv.classList.add("parameter-enabled-toggle");
        paramDiv.innerHTML =
            '<label class="component-toggle" aria-label="' +
            item.DESCRIPTION +
            '\">' +
            '<input type="checkbox" id="' +
            item.id +
            '" ' +
            checked +
            ' oninput=\'sendParameterValue("' +
            item.FULL_PATH +
            '", "i", this.checked)\' />' +
            '<span class="component-toggle-track"><span class="component-toggle-thumb"></span></span>' +
            "</label>";
    } else {
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
    }

    return paramDiv;
}

function createEnumParameterEditor(item, level) {
    const paramDiv = document.createElement("div");
    paramDiv.classList.add("parameter");
    let options = "";
    const enumValues = item.RANGE[0].VALS || {};
    const currentValue = Array.isArray(item.VALUE) ? item.VALUE[0] : item.VALUE;
    for (const key in enumValues) {
        if (!Object.prototype.hasOwnProperty.call(enumValues, key)) {
            continue;
        }
        const label = enumValues[key];
        const numericKey = Number(key);
        const selected =
            currentValue === label || Number(currentValue) === numericKey ? "selected" : "";
        options +=
            '<option value="' +
            numericKey +
            '" ' +
            selected +
            ">" +
            label +
            "</option>";
    }
    paramDiv.innerHTML =
        "<label>" +
        item.DESCRIPTION +
        '</label><select id="' +
        item.id +
        '" oninput=\'sendParameterValue("' +
        item.FULL_PATH +
        '", "i", Number(this.value))\'>' +
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