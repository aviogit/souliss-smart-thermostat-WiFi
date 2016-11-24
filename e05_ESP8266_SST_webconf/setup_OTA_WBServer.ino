#include "constants.h"

void setup_OTA_WBServer(){




  // Init the OTA + WebServer
  // Set Hostname.
  String hostNAME(HOSTNAME);
  WiFi.hostname(hostNAME);
  ArduinoOTA.onStart([]() { events.send("Update Start", "ota"); });
  ArduinoOTA.onEnd([]() { events.send("Update End", "ota"); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    char p[32];
    sprintf(p, "Progress: %u%%\n", (progress/(total/100)));
    events.send(p, "ota");
  });
  ArduinoOTA.onError([](ota_error_t error) {
    if(error == OTA_AUTH_ERROR) events.send("Auth Failed", "ota");
    else if(error == OTA_BEGIN_ERROR) events.send("Begin Failed", "ota");
    else if(error == OTA_CONNECT_ERROR) events.send("Connect Failed", "ota");
    else if(error == OTA_RECEIVE_ERROR) events.send("Recieve Failed", "ota");
    else if(error == OTA_END_ERROR) events.send("End Failed", "ota");
  });
  ArduinoOTA.setHostname((const char *)hostNAME.c_str());
  ArduinoOTA.begin();

  MDNS.addService("http","tcp",80);

  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  events.onConnect([](AsyncEventSourceClient *client){
    client->send("hello!",NULL,millis(),1000);
  });
  server.addHandler(&events);

  server.addHandler(new SPIFFSEditor(http_username,http_password));

  server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.printf("GET /heap");
    request->send(200, "text/plain", String(ESP.getFreeHeap()));
  });
  
  server.on("/away_ON", HTTP_GET, [](AsyncWebServerRequest *request){
    B_away_WBS=1;
    Serial.printf("GET /away_ON");
    request->send(200, "text/plain", String(ESP.getFreeHeap()));
  });
  
  server.on("/away_OFF", HTTP_GET, [](AsyncWebServerRequest *request){
    B_away_WBS=0;
    Serial.printf("GET /away_OFF");
    request->send(200, "text/plain", String(ESP.getFreeHeap()));
  });
  
  server.on("/all", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.printf("GET /all");
    String json = "{";
    json += "\"heap\":"+String(ESP.getFreeHeap());
    json += ", \"S_temperature_WBS\":"+S_temperature_WBS;
    json += ", \"S_setpoint_WBS\":"+S_setpoint_WBS;
    json += ", \"S_humidity_WBS\":"+S_humidity_WBS;
    json += ", \"S_relestatus_WBS\":" +String(S_relestatus_WBS);
    json += ", \"B_is_away_WBS\":" +String(B_is_away_WBS);
    json += "}";
    Serial.printf("Json: \n");
    request->send(200, "application/json", json);
  });
  
  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.htm");

  server.onNotFound([](AsyncWebServerRequest *request){
    Serial.printf("NOT_FOUND: ");
    if(request->method() == HTTP_GET)
      Serial.printf("GET");
    else if(request->method() == HTTP_POST)
      Serial.printf(" POST http://%s%s\n", "Host",request->host().c_str());
      //Serial.printf("POST");
    else if(request->method() == HTTP_DELETE)
      Serial.printf("DELETE");
    else if(request->method() == HTTP_PUT)
      Serial.printf("PUT");
    else if(request->method() == HTTP_PATCH)
      Serial.printf("PATCH");
    else if(request->method() == HTTP_HEAD)
      Serial.printf("HEAD");
    else if(request->method() == HTTP_OPTIONS)
      Serial.printf("OPTIONS");
    else
      Serial.printf("UNKNOWN");
    Serial.printf(" http://%s%s\n", request->host().c_str(), request->url().c_str());

    if(request->contentLength()){
      Serial.printf("_CONTENT_TYPE: %s\n", request->contentType().c_str());
      Serial.printf("_CONTENT_LENGTH: %u\n", request->contentLength());
    }

    int headers = request->headers();
    int i;
    for(i=0;i<headers;i++){
      AsyncWebHeader* h = request->getHeader(i);
      Serial.printf("_HEADER[%s]: %s\n", h->name().c_str(), h->value().c_str());
    }

    int params = request->params();
    for(i=0;i<params;i++){
      AsyncWebParameter* p = request->getParam(i);
      if(p->isFile()){
        Serial.printf("_FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
      } else if(p->isPost()){
        Serial.printf("_POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
      } else {
        Serial.printf("_GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
      }
    }

    request->send(404);
  });
  server.onFileUpload([](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
    if(!index)
      Serial.printf("UploadStart: %s\n", filename.c_str());
    Serial.printf("%s", (const char*)data);
    if(final)
      Serial.printf("UploadEnd: %s (%u)\n", filename.c_str(), index+len);
  });
  server.onRequestBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
    if(request->method() == HTTP_POST && request->url() == "/save")
      if(!index)
        Serial.printf("Request : %u\n",request);
        Serial.printf("BodyStart: %u\n", index);
        Serial.printf("scrivo file: sst_crono_matrix.json");
        Serial.printf("%s", (const char*)data);
        String S_filena_WBS = "/sst_crono_matrix.json";
        fsUploadFile = SPIFFS.open(S_filena_WBS, "w");
        if (!fsUploadFile) 
          Serial.println("file open failed");
        fsUploadFile.printf("%s",(const char*)data);  
      if(index + len == total)
        Serial.printf("BodyEnd: %u\n", total);
        fsUploadFile.close();
      
  });
  
  server.begin();
  ///////////////////////////////////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////////////////////////////////
}  