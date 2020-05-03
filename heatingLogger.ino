#include <SPI.h>
#include <Ethernet.h>
#include <TextFinder.h>

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; //physical mac address

// WebServer 
EthernetServer server(80);

// Client
EthernetClient heizungClient;
TextFinder finder( heizungClient );
IPAddress heizung(192,168,178,116); // Hier die IP-Adresse der Heizung eintragen http://192.168.178.116:8080
uint16_t heizungPort = 8080;

// Daten die von der Heizung gessammelt werden:
float outdoorTemperature;
float lowerSolarStorageTemperature;
float collectorTemperature;
float collectorPump;

void setup(){  
  Serial.begin(9600);
  // wir versuchen es mit einer automatisch zugewiesenen IP
  if (Ethernet.begin(mac) == 0){
    Serial.println("keine IP. STOP.");
    for (;;){}
  }
  Serial.print("Meine IP: ");
  Serial.println(Ethernet.localIP());  
  server.begin();
}

void loop() {   
  // WebServer Antwort
  EthernetClient client = server.available();
  if (client) {    
     Serial.println("Anfrage eines Browsers erhalten."); 
     collectData();

     while (client.connected()) {
        if (client.available()){    

            char c = client.read();
            Serial.write(c);
            if (c == '\n') {
              
            Serial.println("Sende Antwort."); 
            client.println("HTTP/1.1 200 OK"); //send new page
            client.println("Content-Type: text/html");
            client.println("Connection: close");
            //client.println("Refresh: 5"); // alle 5 sek soll die Seite neu geladen werden.
            client.println();

            client.println("<HTML>");
            client.println("<HEAD>");
            client.println("<TITLE>Heizung test page</TITLE>");
            client.println("</HEAD>");
            client.println("<BODY>");

            // jetzt geben wir die Daten aus
            client.println("<div><span>Außentemperatur:</span><span>");            
            client.println(outdoorTemperature,1);
            client.println("</span><span>°C</span></div>");

            client.println("<div><span>Temperatur Solarspeicher unten:</span><span>");            
            client.println(lowerSolarStorageTemperature,0);
            client.println("</span><span>°C</span></div>");
            
            client.println("<div><span>Kollektortemperatur:</span><span>");            
            client.println(collectorTemperature,0);
            client.println("</span><span>°C</span></div>");
            
            client.println("<div><span>Kollektorpumpe:</span><span>");            
            client.println(collectorPump,0);
            client.println("</span><span>%</span></div>");
            
            // evtl. noch weitere
            
            client.println("</BODY>");
            client.println("</HTML>");
            break;
            }
      }      
    }    
    delay(1);
    client.stop();
    Serial.println("Der Browser ist wieder weg.");
    Serial.println(); 
  }
}

void collectData(){
    getOutdoorTemperature();
    getLowerSolarStorageTemperature();
    getCollectorTemperature();
    getCollectorPump();
    // hier evtl. noch Datum/Uhrzeit ergänzen
  
}

void getOutdoorTemperature(){
  outdoorTemperature = getFloatEta("/user/var/120/10221/0/0/12197",-1);
  Serial.print("Außentemperatur: ");
  Serial.print(outdoorTemperature,1);  
  Serial.println("°C"); 
}

void getLowerSolarStorageTemperature(){
  lowerSolarStorageTemperature = getFloatEta("/user/var/120/10221/0/0/12185",-1);
  Serial.print("Temperatur Solarspeicher unten: ");
  Serial.print(lowerSolarStorageTemperature,0);  
  Serial.println("°C"); 
}

void getCollectorTemperature(){
  collectorTemperature = getFloatEta("/user/var/120/10221/0/0/12275",-1);
  Serial.print("Kollektortemperatur: ");
  Serial.print(collectorTemperature,0); 
  Serial.println("°C"); 
}

void getCollectorPump(){
  collectorPump = getFloatEta("/user/var/120/10221/0/0/12278",-1);
  Serial.print("Kollektorpumpe: ");
  Serial.print(collectorPump,0);  
  Serial.println("%");
}


float getFloatEta(String uri, float defaultValue){
  float result = defaultValue;
  Serial.print("Daten abfragen: ");
  Serial.println(uri);
  if (heizungClient.connect(heizung, heizungPort)) {  
    heizungClient.println("GET " + uri + " HTTP/1.0");    
    heizungClient.println();    
    if (heizungClient.connected()){
      char fromValue[] = "strValue=\"";
      if(finder.find(fromValue) ){  
        result = finder.getFloat(',');         
      } else {
        Serial.println("Wert kann nicht gefunden werden.");
      }
      char fromDecPlaces[] = "decPlaces=\"";
      if(finder.find(fromDecPlaces) ){ 
        long decValues = finder.getValue(); 
        result = result / pow(10,decValues);
      }
    }
  } else {
    Serial.println("connection failed");    
  }  
  Serial.println("Verbindung zur Heizung getrennt.");
  heizungClient.stop(); 
  heizungClient.flush();  
  return result;
}
