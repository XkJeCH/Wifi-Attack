#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>

#define DBG_OUTPUT_PORT Serial

const byte DNS_PORT = 53;
const char *ssid = "True_Wifi_60000Thz";

IPAddress apIP(192, 168, 1, 1);

DNSServer dnsServer;

ESP8266WebServer webServer(80);

File dataFile; // ประกาศตัวแปรสำหรับไฟล์ข้อมูล

void setup() {
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(ssid);

  // if DNSServer is started with "*" for domain name, it will reply with
  // provided IP to all DNS request
  dnsServer.start(DNS_PORT, "*", apIP);

  //start debug port
  DBG_OUTPUT_PORT.begin(115200);
  DBG_OUTPUT_PORT.print("\n");
  DBG_OUTPUT_PORT.setDebugOutput(true);
  SPIFFS.begin();

  //redirect all traffic to index.html
  webServer.onNotFound([]() {
    if (!handleFileRead(webServer.uri())) {
      const char *metaRefreshStr = "<head><meta http-equiv=\"refresh\" content=\"0; url=http://192.168.1.1/index.html\" /></head><body><p>redirecting...</p></body>";
      webServer.send(200, "text/html", metaRefreshStr);
    }
  });

  webServer.begin();

  // Open the data file for writing
  dataFile = SPIFFS.open("/data.txt", "w");
  if (!dataFile) {
    Serial.println("Failed to open data file for writing");
  }
}

void loop() {
  dnsServer.processNextRequest();
  webServer.handleClient();
  if (webServer.args() != 0) {
    showClients();
    for (int i = 0; i < webServer.args(); i++) {
      Serial.println(webServer.argName(i) + ": " + webServer.arg(i));
    }
    delay(5000);
  }
}

void showClients() {
  struct station_info *stat_info;
  stat_info = wifi_softap_get_station_info();
  uint8_t client_count = wifi_softap_get_station_num();
  String str = "Number of clients = ";
  str += String(client_count);
  str += "\r\nList of clients : \r\n";
  int i = 1;
  while (stat_info != NULL) {
    str += "Station #";
    str += String(i);
    str += " : ";
    str += String(stat_info->bssid[0], HEX);
    str += ":";
    str += String(stat_info->bssid[1], HEX);
    str += ":";
    str += String(stat_info->bssid[2], HEX);
    str += ":";
    str += String(stat_info->bssid[3], HEX);
    str += ":";
    str += String(stat_info->bssid[4], HEX);
    str += ":";
    str += String(stat_info->bssid[5], HEX);
    str += "\r\n";
    i++;
    stat_info = STAILQ_NEXT(stat_info, next);
  }
  Serial.println(str);

  // Open the data file for appending
  dataFile = SPIFFS.open("/data.txt", "a");
  if (dataFile) {
    // Write data to the file
    dataFile.println(str);
    dataFile.close(); // Close the file
  } else {
    Serial.println("Failed to open data file for appending");
  }
}

String getContentType(String filename) {
  if (webServer.hasArg("download")) return "application/octet-stream";
  else if (filename.endsWith(".htm")) return "text/html";
  else if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".png")) return "image/png";
  else if (filename.endsWith(".gif")) return "image/gif";
  else if (filename.endsWith(".jpg")) return "image/jpeg";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".xml")) return "text/xml";
  else if (filename.endsWith(".pdf")) return "application/x-pdf";
  else if (filename.endsWith(".zip")) return "application/x-zip";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

bool handleFileRead(String path) {
//  DBG_OUTPUT_PORT.println("handleFileRead: " + path);
  if (path.endsWith("/")) path += "index.html";
  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) {
    if (SPIFFS.exists(pathWithGz))
      path += ".gz";
    File file = SPIFFS.open(path, "r");
    size_t sent = webServer.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}
