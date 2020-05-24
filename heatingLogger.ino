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
#define heizungPort 8080

// Daten die von der Heizung gessammelt werden:
float outdoorTemperature;
float lowerSolarStorageTemperature;
float collectorTemperature;
float collectorPump;
float Kalorien=0;
#define chipSelect 4
bool SDCardInitialisiert = false;
#define DELAY 10 // Minuten
const char* fileName = "Solor_1.csv";

void setup(){  
  Serial.begin(9600);
  setupEthernet();
  setupSDCard();
  setupLcd();
}

void setupEthernet(){
  // wir versuchen es mit einer automatisch zugewiesenen IP
  if (Ethernet.begin(mac) == 0){
    Serial.println(F("keine IP. STOP."));
   // for (;;){}
  }
  Serial.print(F("Meine IP: "));
  Serial.println(Ethernet.localIP());  
  server.begin();
}

void setupSDCard(){
   Serial.print(F("Initializing SD card..."));

  // see if the card is present and can be initialized:
  SDCardInitialisiert = SD.begin(chipSelect);
  if (!SDCardInitialisiert) {
    Serial.println(F("Card failed, or not present"));
  
    // don't do anything more:
    return;
   
  }
  Serial.println(F("card initialized."));

 File dataFile = SD.open(fileName, FILE_WRITE);
   if (dataFile) {
     dataFile.println(F("AussenTemp ; SpTemp ; KollTemp ; % Pumpe") );
     dataFile.println("");
     dataFile.close();
    } 
}

void setupLcd(){
  lcd.begin(20,4);
  lcd.setCursor(0,0);
  lcd.print(F("Init"));
  Serial.println(F("LCD Initialisiert."));
}


int lastRequest = millis() - DELAY * 60000;

void loop() {   
  if (millis() - lastRequest >= DELAY * 60000) {
    collectData();

      /*der Faktor 2/3000 ergibt sich aus der Gleichung für die
      * Wärmemenge aus Massenstrom Q =6l/min/%pumpenleistung (/100)
      * über eine Zeit von 10 Minuten (*10) Umrechnung kcal in kJ (*3.7)
      * und der Umrechnung in kWh (/3600)
           */
    Kalorien=(collectorTemperature-2-lowerSolarStorageTemperature)*collectorPump/16.2;

    writeToSdCard();
    refreshLCD();
    lastRequest = millis();
  }
  handleBrowseRequest();
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
  Serial.print(F("Außentemp.: "));
  Serial.print(outdoorTemperature,1);  
  Serial.println(F("°C")); 
}

void getLowerSolarStorageTemperature(){
  lowerSolarStorageTemperature = getFloatEta(F("/user/var/120/10221/0/0/12185"),-1);
  Serial.print(F("Temp. Speicher unten: "));
  Serial.print(lowerSolarStorageTemperature,0);  
  Serial.println(F("°C")); 
}

void getCollectorTemperature(){
  collectorTemperature = getFloatEta(F("/user/var/120/10221/0/0/12275"),-1);
  Serial.print(F("Kollektortemp.: "));
  Serial.print(collectorTemperature,0); 
  Serial.println(F("°C")); 
}

void getCollectorPump(){
  collectorPump = getFloatEta(F("/user/var/120/10221/0/0/12278"),-1);
  Serial.print(F("Pumpe: "));
  Serial.print(collectorPump,0);  
  Serial.println(F("%"));
}


float getFloatEta(String uri, float defaultValue){
  float result = defaultValue;
  Serial.print(F("Daten abfragen: "));
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
    Serial.println(F("connection failed"));    
  }  
  Serial.println(F("Verbindung zur Heizung getrennt."));
  heizungClient.stop(); 
  heizungClient.flush();  
  return result;
}


void handleBrowseRequest(){
  EthernetClient client = server.available();
  while (client.connected()) {
    if (client.available()){    

        char c = client.read();
        Serial.write(c);
        if (c == '\n') {
          
        Serial.println(F("Sende Antwort.")); 
        client.println(F("HTTP/1.1 200 OK")); //send new page
        client.println(F("Content-Type: text/html"));
        client.println(F("Connection: close"));
        client.println();

        client.println(F("<HTML>"));
        client.println(F("<BODY>"));

        client.println(F("<div><span>W&auml;rmemenge bisher in kWh:</span><span>"));            
        client.println(Kalorien,1);
        client.println(F("</span><span>°C</span></div>"));
        
        client.println(F("</BODY>"));
        client.println(F("</HTML>"));
        break;
        }              
      }    
    }
    client.stop();
}

void writeToSdCard(){
  if(SDCardInitialisiert){
    File dataFile = SD.open(fileName, FILE_WRITE);
    if (dataFile) {
      dataFile.print(outdoorTemperature);
      dataFile.print(F("   ;"));
      dataFile.print(lowerSolarStorageTemperature);
      dataFile.print(F("   ;"));
      dataFile.print( collectorTemperature);
      dataFile.print(F("   ;"));
      dataFile.println(collectorPump);
      dataFile.close();
      Serial.print(F("Data written to "));
    }  else {
      Serial.print(F("error opening "));
    }
    Serial.println(fileName);
  }
}

void refreshLCD(){
  lcd.setCursor(0,0);
  lcd.print(Kalorien);
  lcd.print(F("°C"));
}
