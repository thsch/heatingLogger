/* letzte funktionsfähige Version war Version 5. Die Solardaten wurden 
   in der Schleife alle 10 Minuten abgefragt. 
   Dazu war nach der Abfrage und Messwertspeicherung ein Delay
   von 10 Minuten eingebaut.
    In der Version 6 soll dafür eine Bedingung mit gesammelten
    MillisekundenNachteil verwendet werden.
    Vers. / Versuch Einbindung LCD Anzeige
*/



#include <SPI.h>
#include <Ethernet.h>
#include <TextFinder.h>
#include <SD.h>
#include <LiquidCrystal.h>


byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; //physical mac address

// WebServer 
EthernetServer server(80);

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(2,3,5,6,7,8);


// Client
EthernetClient heizungClient;
TextFinder finder( heizungClient );
IPAddress heizung(192,168,2,108); // Hier die IP-Adresse der Heizung eintragen http://192.168.178.116:8080
uint16_t heizungPort = 8080;

// Daten die von der Heizung gessammelt werden:
float outdoorTemperature;
float lowerSolarStorageTemperature;
float collectorTemperature;
float collectorPump;
float Kalorien=0;
const int chipSelect = 4;
bool SDCardInitialisiert = false;
//long lastMillis=0;
int anzahl=0;

void setup(){  
  Serial.begin(9600);
  // wir versuchen es mit einer automatisch zugewiesenen IP
  if (Ethernet.begin(mac) == 0){
    Serial.println("keine IP. STOP.");
   // for (;;){}
  }
  Serial.print("Meine IP: ");
  Serial.println(Ethernet.localIP());  
  server.begin();
 // lastMillis=millis();

    Serial.print(F("Initializing SD card..."));

  // see if the card is present and can be initialized:
  SDCardInitialisiert = SD.begin(chipSelect);
  if (!SDCardInitialisiert) {
    Serial.println(F("Card failed, or not present"));
  
    // don't do anything more:
    return;
   
  }
  Serial.println("card initialized.");

 File dataFile = SD.open(F("Solar_1.txt"), FILE_WRITE);
   if (dataFile) {
     dataFile.println(F("AussenTemp ; SpTemp ; KollTemp ; % Pumpe") );
     dataFile.println("");
     dataFile.close();
    } 
}

void loop() {   
  // WebServer Antwort
  EthernetClient client = server.available();
      //Serial.println("Anfrage eines Browsers erhalten."); 

 if (anzahl==600){
     collectData();

     Kalorien=(collectorTemperature-2-lowerSolarStorageTemperature)*
     collectorPump/16.2;

     /*der Faktor 2/3000 ergibt sich aus der Gleichung für die
      * Wärmemenge aus Massenstrom Q =6l/min/%pumpenleistung (/100)
      * über eine Zeit von 10 Minuten (*10) Umrechnung kcal in kJ (*3.7)
      * und der Umrechnung in kWh (/3600)
           */

if(SDCardInitialisiert){
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  File dataFile = SD.open(F("Solar_1.txt"), FILE_WRITE);

  // if the file is available, write to it:
  if (dataFile) {
    dataFile.print(outdoorTemperature);
    dataFile.print("   ;");
    dataFile.print(lowerSolarStorageTemperature);
    dataFile.print("   ;");
    dataFile.print( collectorTemperature);
    dataFile.print("   ;");
    dataFile.println(collectorPump);


    
    dataFile.close();
    }
 // if the file isn't open, pop up an error:
  else {
    Serial.println(F("error opening Solar_1"));
    }
   }

  anzahl=0; 
  }
   

     while (client.connected()) {
        if (client.available()){    

            char c = client.read();
            Serial.write(c);
            if (c == '\n') {
              
            Serial.println("Sende Antwort."); 
            client.println(F("HTTP/1.1 200 OK")); //send new page
            client.println(F("Content-Type: text/html"));
            client.println(F("Connection: close"));
            //client.println("Refresh: 5"); // alle 5 sek soll die Seite neu geladen werden.
            client.println();

            client.println("<HTML>");
            client.println("<HEAD>");
            client.println(F("<TITLE>Heizung test page</TITLE>"));
            client.println("</HEAD>");
            client.println("<BODY>");

            // jetzt geben wir die Daten aus
            client.println(F("<div><span>Waermemenge bisher in kWh:</span><span>"));            
            client.println(Kalorien,1);
            client.println(F("</span><span>°C</span></div>"));

            /*client.println(F("<div><span>Temperatur Solarspeicher unten:</span><span>"));            
            client.println(lowerSolarStorageTemperature,0);
            client.println(F("</span><span>°C</span></div>"));
            
            client.println(F("<div><span>Kollektortemperatur:</span><span>"));            
            client.println(collectorTemperature,0);
            client.println(F("</span><span>°C</span></div>"));
            
            client.println(F("<div><span>Kollektorpumpe:</span><span>"));            
            client.println(collectorPump,0);
            client.println(F("</span><span>%</span></div>"));*/
            
            // evtl. noch weitere
            
            client.println("</BODY>");
            client.println("</HTML>");
            break;
            }  
            
      } 
   
    //Serial.println(F("Der Browser ist wieder weg."));
   // Serial.println();   
    }   

   
   //lastMillis=millis();
     delay(1000);
   anzahl=anzahl+1;   
 client.stop();
  }
  

  

void collectData(){
    getOutdoorTemperature();
    getLowerSolarStorageTemperature();
    getCollectorTemperature();
    getCollectorPump();
    // hier evtl. noch Datum/Uhrzeit ergänzen
  
}

void getOutdoorTemperature(){
  outdoorTemperature = getFloatEta(F("/user/var/120/10221/0/0/12197"),-1);
  Serial.print("Außentemp.: ");
  Serial.print(outdoorTemperature,1);  
  Serial.println("°C"); 
}

void getLowerSolarStorageTemperature(){
  lowerSolarStorageTemperature = getFloatEta(F("/user/var/120/10221/0/0/12185"),-1);
  Serial.print("Temp. Speicher unten: ");
  Serial.print(lowerSolarStorageTemperature,0);  
  Serial.println("°C"); 
}

void getCollectorTemperature(){
  collectorTemperature = getFloatEta(F("/user/var/120/10221/0/0/12275"),-1);
  Serial.print("Kollektortemp.: ");
  Serial.print(collectorTemperature,0); 
  Serial.println("°C"); 
}

void getCollectorPump(){
  collectorPump = getFloatEta(F("/user/var/120/10221/0/0/12278"),-1);
  Serial.print("Pumpe: ");
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
        Serial.println("kein Wert");
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
  Serial.println(F("Verbindung zur Heizung getrennt."));
  heizungClient.stop(); 
  heizungClient.flush();  
  return result;
}
