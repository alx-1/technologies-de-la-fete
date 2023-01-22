#include <WebServer.h>
#include <ESPmDNS.h>
#include <DNSServer.h>
#include "webpages.h"
const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 1, 1);
DNSServer dnsServer;

WebServer server(80);


void portalInit(){
  //automatically connect using saved credentials if they exist
    //If connection fails it starts an access point with the specified name
    if(wm.autoConnect("CajitaAbierta")){
        Serial.println("connected...yeey :)");
    }
    else {
        Serial.println("Configportal running");
    }
}


void startPortal(){
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP("CajitaAbierta");
  // if DNSServer is started with "*" for domain name, it will reply with
  // provided IP to all DNS request
  dnsServer.start(DNS_PORT, "*", apIP);
  // replay to all requests with same HTML
  server.onNotFound([]() {
  server.send(200, "text/html", contentHome);
  });
  server.begin();
}

void serverListen(){
  dnsServer.processNextRequest();
  server.handleClient();
}
