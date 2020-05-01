#include <SPI.h>
#include <Ethernet.h>
#include <TextFinder.h>

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; //physical mac address

// WebServer 
EthernetServer server(80);

// Client
EthernetClient heizungClient;
TextFinder finder( heizungClient );
IPAddress heizung(192,168,178,66); // Hier die IP-Adresse der Heizung eintragen http://raspberrypi:8080
uint16_t heizungPort = 8080;

// Daten die von der Heizung gessammelt werden:
char resultValue[100];


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
            client.println("Refresh: 5"); // alle 5 sek soll die Seite neu geladen werden.
            client.println();

            client.println("<HTML>");
            client.println("<HEAD>");
            client.println("<TITLE>Heizung test page</TITLE>");
            client.println("</HEAD>");
            client.println("<BODY>");

            // jetzt geben wir die Daten aus
            client.println("<div><span>Wert:</span><span>");            
            client.println(resultValue);
            client.println("</span></div>");

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
  if (heizungClient.connect(heizung, heizungPort)) {
    Serial.println("Daten von Heizung abfragen.");
    collectFromPage1();
    // hier evtl. weitere Seiten abfragen    
  } else {
    Serial.println("connection failed");
    Serial.println();
  }
  
  Serial.println("Verbindung zur Heizung getrennt.");
  heizungClient.stop();  
}

void collectFromPage1(){
  heizungClient.println("GET / HTTP/1.0");    
  heizungClient.println();
  while(heizungClient.connected() && !heizungClient.available()) delay(1); //warten auf Antwort
  while (heizungClient.connected() || heizungClient.available()) { //Daten empfangen
  
    // Wert auslesen. Diesen Block kopieren, um alle Daten der Seite auszulesen  
    char from[] = "<body>";
    char to[] = "</body>";
    if(finder.find(from) ){  // suche in der Antwort nach dem Wert <body>   
      memset(resultValue,0,sizeof(resultValue)); // alte Daten im Speicher löschen (nur bei Strings nötig)

      // Jetzt schneiden wir alles aus was zwischen <body> und </body> steht und speichern das als Ergebnis
      // finder hat auch Methoden wie getValue um Zahlenwerte zu lesen. komplette Doku: https://github.com/tardate/TextFinder
      finder.getString(to,resultValue,sizeof(resultValue)); 

      // zur kontrolle mal auf der Konsole ausgeben
      Serial.print("Wert: ");
      Serial.println(resultValue);
    }
  }

}
