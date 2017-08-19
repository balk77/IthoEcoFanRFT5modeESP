// web update URL:
// http://esp8266-webupdate.local/update

#include <SPI.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "IthoCC1101.h"
#include "IthoPacket.h"
#include <SoftwareSerial.h>
#define INTERVAL 5000
#define MAX_DATA_ERRORS 50 //max of errors, reset after them


// From BasicOTA.ino
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
//#include <ArduinoOTA.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266WebServer.h>


// Update these with values suitable for your network.
const char* host = "esp8266-webupdate-ITHO";
// MAC address 2c:3a:e8:08:e3:7f
// Open http://esp8266-webupdate-ITHO.local/update in your browser
const char* ssid = "xxxx";
const char* password = "xxxx";
const char* mqtt_server = "192.168.0.9";
ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;


long previousMillis = 0;
int errorCount = 0;
WiFiClient espClient;
IthoCC1101 rf;
IthoPacket packet;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
//int msg;
int value = 0;
const char* outtopic = "/zolder/itho/remote";
const char* inTopic = "/hass/itho";
char commando;
char outtopic2[50];



// float to string
// http://forum.arduino.cc/index.php?topic=175478.0
char *f2s(float f, int p){
  char * pBuff;                         // use to remember which part of the buffer to use for dtostrf
  const int iSize = 10;                 // number of bufffers, one for each float before wrapping around
  static char sBuff[iSize][20];         // space for 20 characters including NULL terminator for each float
  static int iCount = 0;                // keep a tab of next place in sBuff to use
  pBuff = sBuff[iCount];                // use this buffer
  if(iCount >= iSize -1){               // check for wrap
    iCount = 0;                         // if wrapping start again and reset
  }
  else{
    iCount++;                           // advance the counter
  }
  return dtostrf(f, 0, p, pBuff);       // call the library function
}


void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  
 // String commando = String((char *)byteArray);

  payload[length] = '\0';
  String commando = String((char*)payload);


  //char *commando = (char*)payload;

  for (int i = 0; i < length; i++) {
    
    //Serial.print(" lala");
    Serial.print((char)payload[i]);
  }
  Serial.println();
  Serial.println(commando);
  
  if (commando == "away") {
    rf.sendCommand(IthoAway);
  }
  else if (commando == "low") {
    rf.sendCommand(IthoLow);
  }
  else if (commando == "medium") {
    rf.sendCommand(IthoMedium);
  }
  else if (commando == "high") {
    rf.sendCommand(IthoHigh);  
  }
  else if (commando == "power") {
    rf.sendCommand(IthoFullPower);
  }
  else if (commando == "timer") {
    rf.sendCommand(IthoTimer1);   
  }
  else if (commando == "restart") {
    ESP.restart();
  }

  
  commando = "";

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is acive low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }

}


void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(),"/zolder/itho/remote",0,0,"false")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      sprintf(outtopic2,"%sstatus",outtopic);
      //Serial.println(outtopic2);
      client.publish(outtopic2, "initializing");
      
      // ... and resubscribe
      client.subscribe(inTopic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  Serial.println("");
  Serial.println("ESP8266 remote for Itho ventilation unit");
  Serial.println("See https://gathering.tweakers.net/forum/list_messages/1690945 for more information!");
  Serial.println("Setting up CC1101 module...");
  rf.init();
  Serial.println("CC1101 transmitter is now ready!");
  Serial.println("");
  rf.sendCommand(IthoJoin);
  Serial.println("");

    unsigned long previousMillis = millis();
  while (millis() - previousMillis < 10000)
    delay(1000);
  Serial.println("Setup finished");
  Serial.println("");

    // From BasicOTA.ino
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Start the webserver
  MDNS.begin(host);

  httpUpdater.setup(&httpServer);
  httpServer.begin();

  MDNS.addService("http", "tcp", 80);
  Serial.printf("HTTPUpdateServer ready! Open http://%s.local/update in your browser\n", host);

  


}

void loop() {
  // keep webserver active
  httpServer.handleClient();

  
  // From BasicOTA.ino
//  ArduinoOTA.handle();

  if (!client.connected()) {
    reconnect();
  }
  //client.publish("/nh11/nodemcuitho/$online","true");
  client.loop();
  //delay(1000);

  long now = millis();
  if (now - lastMsg > INTERVAL) {
    lastMsg = now;
    ++value;
    snprintf (msg, 75, "hello world #%ld", value);

    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish(outtopic, "true");

  
  }

  
}

