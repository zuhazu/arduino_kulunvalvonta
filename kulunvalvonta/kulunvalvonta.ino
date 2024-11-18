#include <base64.hpp>

//#include <arduino_base64.hpp>
//#include <b64.h>

#include <HttpClient.h>
#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
//Näytön kirjastot
#include <Wire.h>
#include "rgb_lcd.h"

rgb_lcd lcd;

const int colorR = 255;
const int colorG = 0;
const int colorB = 0;

#define RST_PIN         9          // Configurable, see typical pin layout above
#define SS_PIN          10         // Configurable, see typical pin layout above

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance
WiFiClient client;

//ssid:036-Wifi
//pass:036luokka

char ssid[] = "036-Wifi";
char pass[] = "036luokka";
int keyIndex = 0;
int status = WL_IDLE_STATUS;
//192.168.43.47
IPAddress server(192,168,1,105);

void setup() {
	Serial.begin(9600);		// Initialize serial communications with the PC
	while (!Serial);		// Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
	SPI.begin();			// Init SPI bus
  initLCD();    //Näytön käyttöönotto

  initRFID();
  initWifi();
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
    lcd.setCursor(0, 1);
    lcd.print("Connecting");
    Serial.print("Yritethään yhistää SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);
    delay(5000);
  }
  //tulisiko lisätä timeout?
  lcd.clear();
  lcd.setRGB(0, 255, 0);
  lcd.setCursor(0, 0);
  lcd.print("Connected");
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
  //LCD
  lcd.clear();
    String username = "admin";
    String password = "password";
    String request = "PUT /tag HTTP/1.1\n";
    request = request + "host: 192.168.1.105:8080\n";
    request = request + "content-type: application/json\n";
    request = request + "Connection: keep-alive\n";

    String body = "{\"tag_id\":\"" + card_id + "\", \"room_id\":\"036\"}\n";  // JSON-rakenne, jossa on card_id
    request = request + "Content-Length: " + String(body.length()) + "\n";  // Lisätään body-pituus
    //request = "\n";  // Tyhjä rivi otsikoiden ja bodyn väliin
    char auth[] = "admin:password";  // Muodostetaan 'username:password' yhdistelmä
    char encoded[32];
    //encode_base64 (auth,12,encoded);
    encode_base64((unsigned char *) auth, 14, encoded);
    String authAsString = String(encoded);
    Serial.println("Enkootattu : : : " + authAsString);

    //String temp = "Basic YWRtaW46cGFzc3dvcmQ=";
    
    request = request + "Authorization: Basic " + authAsString;  // Lisätään Basic Auth -otsikko
    request = request + "\n\n";
    request = request + body;
  
  if (client.connect(server, 8080)) {

    Serial.print("connected to server: ");
    Serial.println(server);

    Serial.println(request);

    client.print(request);

    client.println();


    bool isBody = false;
    String body = "";

    while (client.connected() || client.available()) {
      String line = client.readStringUntil('\n'); // Luetaan rivi riviltä

      // Header ja body erotetaan tyhjällä rivillä
      if (!isBody && line == "\r") {
        isBody = true; // Body alkaa tästä
        continue;
      }

      if (isBody) {
        body += line; // Lisätään bodyyn
        break;
      }
    }

    // Tulostetaan pelkkä body
    Serial.println("Response Body:");
    Serial.println(body);
    // Tulostetaan palvelimen vastaus
 //   Serial.println("Server response: " + response);
updateScreenColor(body);
    //Serial.println(body);
    //Serial.println(response);
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

const char base64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

void encode_base64_r(const unsigned char *input, size_t length, char *encoded) {
    int i = 0, j = 0;
    unsigned char triple[3];
    unsigned char output[4];
    while (length--) {
        triple[i++] = *input++;
        if (i == 3) {
            output[0] = (triple[0] >> 2) & 0x3F;
            output[1] = ((triple[0] & 0x03) << 4) | (triple[1] >> 4);
            output[2] = ((triple[1] & 0x0F) << 2) | (triple[2] >> 6);
            output[3] = triple[2] & 0x3F;
            for (i = 0; i < 4; i++) {
                encoded[j++] = base64_table[output[i]];
            }
            i = 0;
        }
    }
    if (i) {
        for (int k = i; k < 3; k++) {
            triple[k] = '\0';
        }
        output[0] = (triple[0] >> 2) & 0x3F;
        output[1] = ((triple[0] & 0x03) << 4) | (triple[1] >> 4);
        output[2] = ((triple[1] & 0x0F) << 2) | (triple[2] >> 6);
        output[3] = triple[2] & 0x3F;
        for (int k = 0; k < i + 1; k++) {
            encoded[j++] = base64_table[output[k]];
        }
        while (i++ < 3) {
            encoded[j++] = '=';
        }
    }
    encoded[j] = '\0';
}

void updateScreenColor(String body) {
  lcd.clear();  // Tyhjennetään näyttö ennen väriä ja tekstiä

  // Tarkistetaan, mikä vastaus on ja muutetaan näytön väri sen mukaan
  if (body == "Logged in") {
    lcd.setRGB(0, 255, 0);  // Vihreä väri (Logged in)
    lcd.setCursor(0, 0);
    lcd.print("Logged in");
  } 
  else if (body == "Access denied") {
    lcd.setRGB(255, 0, 0);  // Punainen väri (Access denied)
    lcd.setCursor(0, 0);
    lcd.print("Access denied");
  } 
  else if (body == "Logged out") {
    lcd.setRGB(255, 255, 255);  // Valkoinen väri (Logged out)
    lcd.setCursor(0, 0);
    lcd.print("Logged out");
  }
} 


