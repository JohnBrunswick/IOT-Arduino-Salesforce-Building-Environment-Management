/*
  Sample code to read temperature and make HTTP PUT
  to service running on Heroku via Node.js.  Code sample created using
  some of the following resources

  - WiFi http://arduino.cc/en/Tutorial/WiFiWebClientRepeating
  - Temp tutorial http://learn.adafruit.com/tmp36-temperature-sensor/using-a-temp-sensor

  The following forum post was helpful in making the repeating wirelesss
  connectivity more stable with the PUTs
  - http://forum.arduino.cc/index.php?topic=170460.0

  More at www.johnbrunswick.com
*/

#include <SoftwareSerial.h>
#include <SPI.h>
#include <WiFi.h>

char ssid[] = "xxxxx"; // your network SSID (name) 
char key[] = "xxxxx"; // your network key
int keyIndex = 0;            // your network key Index number (needed only for WEP)

boolean incomingData = false; // required to ensure consistent connectivity
int status = WL_IDLE_STATUS;

// Initialize the Wifi client library
WiFiClient client;

// server address:
IPAddress server(192,168,1,100);

unsigned long lastConnectionTime = 0;           // last time you connected to the server, in milliseconds
boolean lastConnected = false;                  // state of the connection last time through the main loop
const unsigned long postingInterval = 10*2000;  // delay between updates, in milliseconds

//TMP36 Pin Variables
int sensorPin = 0; //the analog pin the TMP36's Vout (sense) pin is connected to
                        //the resolution is 10 mV / degree centigrade with a
                        //500 mV offset to allow for negative temperatures

void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  Serial.println("------------------");
  Serial.println("Starting - RFID and WiFi POC Test");

  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }  

  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present"); 
    // don't continue:
    while(true);
  } 

  // attempt to connect to Wifi network:
  while ( status != WL_CONNECTED) { 
    Serial.print("Attempting to connect to WEP network, SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, keyIndex, key);

    // wait 10 seconds for connection:
    delay(10000);
  }

  Serial.println("Connected to WiFi...");
  Serial.println("------------------");
  Serial.println("");

  printWifiStatus();
}

void loop() {
  while (client.available()) {
    char c = client.read();
    Serial.write(c);
  }
  if (!client.connected() && lastConnected) {
    Serial.println();
    Serial.println("disconnecting.");
    client.stop();
  }
  if(!client.connected() && (millis() - lastConnectionTime > postingInterval)) {
    sendData();
  }
  lastConnected = client.connected();
}

void sendData() {
  
   //getting the voltage reading from the temperature sensor
   int reading = analogRead(sensorPin);  
   
   // converting that reading to voltage, for 3.3v arduino use 3.3
   float voltage = reading * 5.0;
   voltage /= 1024.0; 
   
   float temperatureC = (voltage - 0.5) * 100 ;  //converting from 10 mv per degree wit 500 mV offset
                                                 //to degrees ((voltage - 500mV) times 100)
   
   // now convert to Fahrenheit
   float temperatureF = (temperatureC * 9.0 / 5.0) + 32.0;

  Serial.println("----------------->");
  Serial.println("Trying to send card data to server...");

  // if you get a connection, report back via serial:
  // change port info as needed, this was used with a local instance via Heroku command line
  if (client.connect(server, 3001)) {
  
    Serial.println("Connected to server...");

    static char dtostrfbuffer[15];
    dtostrf(temperatureF, 8, 2, dtostrfbuffer);

    String feedData = "\n{\"tempdata\" : {\"value\" : \"" + String(dtostrfbuffer) + "\"}}";
    Serial.println("Sending: " + feedData);
    
    client.println("PUT /environment/ HTTP/1.0");

    client.println("Host: 192.168.1.100");
    client.println("Content-Type: application/json");
    client.println("Content-Length: " + String(feedData.length()));
    client.print("Connection: close");
    client.println();
    client.print(feedData);
    client.println();    

    Serial.println("Data has been sent...");
    Serial.println("----------------->");

    delay(2000); 
    
    lastConnectionTime = millis();
        
    while (client.available() && status == WL_CONNECTED) {
  	if (incomingData == false)
	{
          Serial.println();
	  Serial.println("--- Incoming data Start ---");
	  incomingData = true;
	}
	char c = client.read();
	Serial.write(c);
        client.flush();
        client.stop();
    }

    if (incomingData == true) {
      Serial.println("--- Incoming data End");
      incomingData = false; 
    }
    if (status == WL_CONNECTED) {
      Serial.println("--- WiFi connected");
    }
    else {
      Serial.println("--- WiFi not connected");
    }
  } 
  else {
    // if you couldn't make a connection:
    Serial.println("connection failed");
    Serial.println("disconnecting.");
    client.stop();
  }
}

void printWifiStatus() {
  // Print the SSID of the network you're attached to:
  Serial.println("------------------");
  Serial.println("WiFi Status...");
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // Print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // Print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("Signal Strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
  Serial.println("------------------");
  Serial.println("");
}
