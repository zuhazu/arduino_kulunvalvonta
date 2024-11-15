#include <arduino_base64.hpp>
#include <b64.h>

#include <HttpClient.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include "rgb_lcd.h"
#include <WiFi.h>


rgb_lcd lcd;

const int colorR = 255;
const int colorG = 0;
const int colorB = 0;

#define RST_PIN         9          // Configurable, see typical pin layout above
#define SS_PIN          10         // Configurable, see typical pin layout above

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance
WiFiClient client;



char ssid[] = "036-Wifi";
char pass[] = "036luokka";
int keyIndex = 0;
int status = WL_IDLE_STATUS;

IPAddress server(192,168,1,109);

void setup() {
	Serial.begin(9600);		// Initialize serial communications with the PC
	while (!Serial);		// Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
	SPI.begin();			// Init SPI bus

  initRFID();
  initWifi();
  initLCD();
  delay(1000);

}

void initLCD()
{
  lcd.begin(16, 2);
  lcd.setRGB(colorR, colorG, colorB);
  lcd.print("Jenninator");
}

void initRFID()
{
  mfrc522.PCD_Init();
  mfrc522.PCD_DumpVersionToSerial();
}

void initWifi()
{
  if (WiFi.status() == WL_NO_SHIELD) 
  {
    Serial.println("WiFi kuori ei ole paikallaan");
    while(true);
  }

  while(status != WL_CONNECTED){
    Serial.print("Yritethään yhistää SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);
    delay(5000);
  }
  //tulisiko lisätä timeout?
  Serial.println("Yhristetty Wifiin");
}

void loop() {
	// Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
	if ( ! mfrc522.PICC_IsNewCardPresent()) {
		return;
	}

	// Select one of the cards
	if ( ! mfrc522.PICC_ReadCardSerial()) {
		return;
	}
String card_id = "";

  // Loop through the UID bytes and append them to the string
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    card_id += String(mfrc522.uid.uidByte[i], HEX);  // Convert each byte to hexadecimal
  }

  // Optionally, make the string uppercase for consistency
  card_id.toUpperCase();
  // Print the UID for debugging
  Serial.print("Card UID: ");
  Serial.println(card_id);
  if (client.connect(server, 8080)) {

    Serial.println("connected to server:");
    Serial.println(server);

    String username = "admin";
    String password = "password";
    String request = "PUT /tag HTTP/1.1\r\n";
    request += "host: 192.168.1.109:8080\r\n";
    request += "content-type: application/json\r\n";

    request += "Connection: close\r\n";

    String body = "{\"tag_id\":\"" + card_id + "\", \"room_id\":\"036\"}";  // JSON-rakenne, jossa on card_id
    request += "Content-Length: " + String(body.length()) + "\r\n";  // Lisätään body-pituus
    request += "\r\n";  // Tyhjä rivi otsikoiden ja bodyn väliin
    request += body;
    String auth = username + ":" + password;  // Muodostetaan 'username:password' yhdistelmä

    
    request += "Authorization: Basic " + auth + "\r\n";  // Lisätään Basic Auth -otsikko

    client.print(request);

    client.println();
    String response = "";
    while (client.available()) {
        response += (char)client.read();  // Luetaan palvelimen vastaus
    }

    // Tulostetaan palvelimen vastaus
    Serial.println("Server response:");
    Serial.println(body);
    Serial.println(response);
    delay(1000);


  }
	// Dump debug info about the card; PICC_HaltA() is automatically called
	//mfrc522.PICC_DumpToSerial(&(mfrc522.uid));
  wifiLoop();
}

void wifiLoop() {
  while (client.available()) {

    char c = client.read();

    Serial.write(c);

  }
  if (!client.connected()) {

    Serial.println();
    client.stop();

  }
}




