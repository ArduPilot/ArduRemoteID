#include "webinterface.h"

#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <WiFiAP.h>
#include <ESPmDNS.h>
#include <Update.h>
#include "parameters.h"
#include "romfs.h"
#include "check_firmware.h"

static WebServer server(80);

/*
  serve files from ROMFS
 */
class ROMFS_Handler : public RequestHandler
{
    bool canHandle(HTTPMethod method, String uri) {
        if (uri == "/") {
            uri = "/index.html";
        }
        uri = "web" + uri;
        if (ROMFS::exists(uri.c_str())) {
            return true;
        }
        return false;
    }

    bool handle(WebServer& server, HTTPMethod requestMethod, String requestUri) {
        if (requestUri == "/") {
            requestUri = "/index.html";
        }
        String uri = "web" + requestUri;
        Serial.printf("handle: '%s'\n", requestUri.c_str());

        // work out content type
        const char *content_type = "text/html";
        const struct {
            const char *extension;
            const char *content_type;
        } extensions[] = {
            { ".js", "text/javascript" },
            { ".jpg", "image/jpeg" },
            { ".css", "text/css" },
        };
        for (const auto &e : extensions) {
            if (uri.endsWith(e.extension)) {
                content_type = e.content_type;
                break;
            }
        }

        auto *f = ROMFS::find_stream(uri.c_str());
        if (f != nullptr) {
            server.sendHeader("Content-Encoding", "gzip");
            server.streamFile(*f, content_type);
            delete f;
            return true;
        }
        return false;
    }

} ROMFS_Handler;

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

typedef struct {
    String name;
    String value;
} json_table_t;

/*
  create a json string from a table
 */
static String json_format(const json_table_t *table, uint8_t n)
{
    String s = "{";
    for (uint8_t i=0; i<n; i++) {
        const auto &t = table[i];
        s += "\"" + t.name + "\" : ";
        s += "\"" + t.value + "\"";
        if (i != n-1) {
            s += ",";
        }
    }
    s += "}";
    return s;
}

/*
  serve files from ROMFS
 */
class AJAX_Handler : public RequestHandler
{
    bool canHandle(HTTPMethod method, String uri) {
        Serial.printf("ajax check '%s'", uri);
        return uri == "/ajax/status.json";
    }

    bool handle(WebServer& server, HTTPMethod requestMethod, String requestUri) {
        if (requestUri != "/ajax/status.json") {
            return false;
        }
        const uint32_t now_s = millis() / 1000;
        const uint32_t sec = now_s % 60;
        const uint32_t min = (now_s / 60) % 60;
        const uint32_t hr = (now_s / 3600) % 24;
        const json_table_t table[] = {
            { "STATUS:VERSION", String(FW_VERSION_MAJOR) + "." + String(FW_VERSION_MINOR) },
            { "STATUS:UPTIME", String(hr) + ":" + String(min) + ":" + String(sec) },
            { "STATUS:FREEMEM", String(ESP.getFreeHeap()) },
        };
        server.send(200, "application/json", json_format(table, ARRAY_SIZE(table)));
        return true;
    }

} AJAX_Handler;

/*
  init web server
 */
void WebInterface::init(void)
{
    Serial.printf("WAP start %s %s\n", g.wifi_ssid, g.wifi_password);
    WiFi.softAP(g.wifi_ssid, g.wifi_password);
    IPAddress myIP = WiFi.softAPIP();

    server.addHandler( &ROMFS_Handler );
    server.addHandler( &AJAX_Handler );

    /*handling uploading firmware file */
    server.on("/update", HTTP_POST, []() {
        server.sendHeader("Connection", "close");
        server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
        ESP.restart();
    }, [this]() {
        HTTPUpload& upload = server.upload();
        if (upload.status == UPLOAD_FILE_START) {
            Serial.printf("Update: %s\n", upload.filename.c_str());
            lead_len = 0;
            if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
                Update.printError(Serial);
            }
        } else if (upload.status == UPLOAD_FILE_WRITE) {
            /* flashing firmware to ESP*/
            if (lead_len < sizeof(lead_bytes)) {
                uint32_t n = sizeof(lead_bytes)-lead_len;
                if (n > upload.currentSize) {
                    n = upload.currentSize;
                }
                memcpy(&lead_bytes[lead_len], upload.buf, n);
                lead_len += n;
            }
            if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
                Update.printError(Serial);
            }
        } else if (upload.status == UPLOAD_FILE_END) {
            // write extra bytes to force flush of the buffer before we check signature
            uint32_t extra = SPI_FLASH_SEC_SIZE+1;
            while (extra--) {
                uint8_t ff = 0xff;
                Update.write(&ff, 1);
            }
            if (!CheckFirmware::check_OTA_next(lead_bytes, lead_len)) {
                Serial.printf("failed firmware check\n");
            } else if (Update.end(true)) {
                Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
            } else {
                Update.printError(Serial);
            }
        }
    });
    Serial.printf("WAP started\n");
    server.begin();
}

void WebInterface::update()
{
    if (!initialised) {
        init();
        initialised = true;
    }
    server.handleClient();
}
