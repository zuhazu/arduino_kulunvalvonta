/**********************************************************************************************************************

Tässä ohjelmassa käytetään Arduino UNO WiFi Rev 2 controlleria, Grove shieldiä, RFID-RC522 lukijaa, Grove LCD- näyttöä ja 1PC passive buzzeria.

Ohjelman tarkoituksena on yhdistää Wifin kautta toisaalla olevaan serveriin, löytää yhteys, jolloin RFID tagia näyttämällä lähetetään tieto serverille.
Serverillä tieto käsitellään ja palautetaan tieto Arduinolle. Riippuen palautuksesta annetaan näyttöön ja Buzzeriin vastaus.

**********************************************************************************************************************/

#include <base64.hpp> // Base koodauksen tuki.
#include <HttpClient.h> // HTTP- pyyntöjen käsittelyyn.
#include <SPI.h> // Otetaan käyttöön SPI- väylät RFID-RC522 - lukijaa varten. 
#include <MFRC522.h> // RFID lukijan tuki.
#include <WiFi.h> // Kirjasto WiFi yhteyksien hallintaan.
#include <Wire.h> // I2C väylän tuki näyttöä varten.
#include "rgb_lcd.h" // Itse bäytön tuki.

rgb_lcd lcd; // Alustetaan näyttö tuki.

// Määritetään näytön väri punaiseksi alussa.
const int colorR = 255;
const int colorG = 0;
const int colorB = 0;

//Määritetään RFID- lukijan pinnit
#define RST_PIN         9          // Reset pinni (RFID- Lukija)
#define SS_PIN          10         // Chip select (RFID- Lukija)

int buzzerPin = 13; //Buzzerin määrittäminen pininlle 13

// Luodaan MFRC522-instanssi RFID-lukijaa varten
MFRC522 mfrc522(SS_PIN, RST_PIN);  // MFRC522-instanssi, käytetään pinnejä 10 (SS) ja 9 (RST)
WiFiClient client; 

// Muistiinpano 036-luokan tunnuksia varten.
// ssid:036-Wifi
// pass:036luokka

// Wifi yhteyden tiedot.
char ssid[] = "036-Wifi";
char pass[] = "036luokka";
int keyIndex = 0;
int status = WL_IDLE_STATUS; // Wi-Fi-yhteyden tila, aluksi lepotilassa

// Määritellään palvelimen IP-osoite (asetetaan riippuen kuka sen nostaa pystyyn.)
IPAddress server(192,168,1,105);

void setup() {
	Serial.begin(9600);		// Alustetaan sarjaportti tietokoneelle
	while (!Serial);		// Odotetaan että portti on valmis (added for Arduinos based on ATMEGA32U4)
	SPI.begin();			// Alustetaan SPI-väylä (tarvitaan MFRC522:n kanssa)
  initLCD();    //Näytön käyttöönotto
  pinMode(buzzerPin, OUTPUT);     //Määritetään buzzerin pinni ulostuloksi.

  initRFID();   //RFID lukijan käyttöönotto
  initWifi();   //WiFi toiminnon käyttöönotto
  delay(1000);

}
// Käynnistäessä Asetetaan näyttöön teksti.
void initLCD()
{
  lcd.begin(16, 2);
  lcd.setRGB(colorR, colorG, colorB);
  lcd.print("Easy Access");
}
// Alustetaan RFID- lukija
void initRFID()
{
  mfrc522.PCD_Init();
  mfrc522.PCD_DumpVersionToSerial(); // Tulostetaan tiedot sarjaporttiin.
}

// Alustetaan WiFi- yhteys
void initWifi()
{
  if (WiFi.status() == WL_NO_SHIELD) // Jos shield ei ole...
  {
    Serial.println("WiFi kuori ei ole paikallaan"); // ...Tulosteteen virhe.
    while(true); 
  }
  // Yritetään muodostaa yhteys WiFi verkkoon. 
  while(status != WL_CONNECTED){
    lcd.setCursor(0, 1); // Asetetaan kursori toiselle riville...
    lcd.print("Connecting"); // ...Jolle printataan tämä.
    Serial.print("Yritethään yhistää SSID: "); // Serialiin pritntataan tietoisesti kirjoitusvirheellä..
    Serial.println(ssid); // ... WiFi:n SSID 
    status = WiFi.begin(ssid, pass); // Yritetään yhdistää WiFi- verkkoon.
    delay(5000);
  }
  
  // Kun on yhdistetty:
  lcd.clear(); // Tyhjätään näyttö.
  lcd.setRGB(0, 255, 0); // Asetetaan näytön väri vihreäksi.
  lcd.setCursor(0, 0); // Kursori ekalle riville.
  lcd.print("Connected"); // Printataan näyttöön tämä.
  Serial.println("Yhristetty Wifiin"); // Printataan sarjaporttiin tämä.
}
/*********************************************************************************************************************************
Tässä osassa tarkastetaan kortin läsnäolo, luodaan HTTP PUT-pyynö serverille, otetaan autentikointi käyttöön, otetaan vastaus serveriltä,
parsitaan vastaus ja siitä riippuen palautetaan näyttöön tietty arvo.
**********************************************************************************************************************************/
void loop() {
	// Tarkastetaan onko kortti läsnä. Jos ei, palataan odottamaan korttia
	if ( ! mfrc522.PICC_IsNewCardPresent()) {
		return;
	}
	// Valitaan kortti ja luetaan sen sarjanumero
	if ( ! mfrc522.PICC_ReadCardSerial()) {
		return;
	}
String card_id = ""; // Luodaan muuttuja kortin UID:tä varten.

  // Käydään UID: tavut ja lisätään ne merkkidataan hexadesimaaleissa.
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    card_id += String(mfrc522.uid.uidByte[i], HEX);  // Muutetaan tavut hexadesimaaleiksi
  }
  card_id.toUpperCase(); // Muutetaan isoiksi kirjaimiksi, jotta olisi yhdenmukainen.
  Serial.print("Card UID: ");
  Serial.println(card_id); // Printataan kortin ID debuggausta varten.
  lcd.clear(); // Pyyhitään näyttö.
    // Määritetään Käyttäjätunnus ja Salasana.
    String username = "admin";
    String password = "password";

    String request = "PUT /api/tag HTTP/1.1\n"; // Muodostetaan HTTP pyyntö PUT menetelmällä + rivinvaihto
    request = request + "host: 192.168.1.105:8080\n"; // Määritetään palvelimen osoite + rivinvaihto
    request = request + "content-type: application/json\n"; // Määritellään sisältötyyppi JSON + rivinvaihto
    request = request + "Connection: keep-alive\n"; // Yhteys pidetään elossa + rivinvaihto

    // Luodaan JSON-muotoinen body, joka sisältää kortin UID:n ja huoneen tunnuksen
    String body = "{\"tag_id\":\"" + card_id + "\", \"room_id\":\"100100\"}\n";  // JSON-rakenne, jossa on card_id
    request = request + "Content-Length: " + String(body.length()) + "\n";  // Lisätään body-pituus HTTP otsikkoon

    // Luodaan perusutentikointi käyttäjätunnus ja salasana.
    char auth[] = "admin:password";  // Muodostetaan 'username:password' yhdistelmä
    char encoded[32]; // Taulukko, johon Base64-koodattu autentikointi tallennetaan.

    encode_base64((unsigned char *) auth, 14, encoded); // Base64-koodaus 'username:password' -yhdistelmästä.
    String authAsString = String(encoded); // Muutetaan Base64-koodattu autentikointi merkkijonoksi.
    Serial.println("Enkootattu : : : " + authAsString); // Tulostetaan Base64-koodattu autentikointi debug-tarkoituksiin

    // Lisätään 'Authorization' -otsikko HTTP-pyyntöön (Basic Authentication).
    request = request + "Authorization: Basic " + authAsString; 
    request = request + "\n\n";
    request = request + body;
  
  // Yritetään muodostaa yhteys palvelimelle
  if (client.connect(server, 8080)) {
    Serial.print("connected to server: ");
    Serial.println(server); // Printataan serveri johon ollaan yhdistetty.
    Serial.println(request); // Tulostetaan pyyntö palvelimen tarkistamista varten.
    client.print(request); // Lähetetään pyyntö palvelimelle
    client.println(); // Pyynnön jälkeen yhjä rivi. 

    bool isBody = false; // Muuttuja, joka seuraa onko palvelimelta tullut body
    String body = ""; // Muuttuja palvelimen vastauksen bodyn tallentamiseen


/**************************************************************************
Tässä parsitaan body, jotta saadaan yksi rivinen vastaus LCD näyttöä varten
***************************************************************************/
    while (client.connected() || client.available()) {
      String line = client.readStringUntil('\n'); // Luetaan rivi riviltä

      // Header ja body erotetaan tyhjällä rivillä
      if (!isBody && line == "\r") {
        isBody = true; // Body alkaa tästä
        continue;
      }
      if (isBody) {
        body += line; // Lisätään bodyyn
        break; // Lopetetaan luenta ensimmäisen rivin jälkeen.
      }
    }
    // Tulostetaan pelkkä body (vastaus palvelimelta)
    Serial.println("Response Body:");
    Serial.println(body);

    updateScreenColor(body); // Päivitetään näyttö palvelimen vastauksen perusteella.
    delay(1000);
  }
wifiLoop(); //Kutsutaan alla olevaa wifi silmukkaa.
}
void wifiLoop() {
  // Luetaan vastaanotettu data Wi-Fi-yhteydeltä, jos sitä on saatavilla.
  while (client.available()) {
    char c = client.read(); // Lue merkki.
    Serial.write(c); // Tulosta merkki sarjaporttiin.
  }
  // Jos yhteys on katkennut, suljetaan yhteys
  if (!client.connected()) {
    Serial.println(); // Tyhjä rivi
    client.stop(); // Suljetaan yhteys
  }
}
/******************************************************************************************************************************
Tämä pätkä ottaa binääri dataa ja muuttaa sen Base64 muotoon. Auttaa datan lähetykseen verkon yli.
*******************************************************************************************************************************/
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

//Tässä kohdassa Asetetaan näytön tila ja annetaan ääni riippuen onko Body "logged in", "logged out" taikka "Access denied"
void updateScreenColor(String body) {
  lcd.clear();  // Tyhjennetään näyttö ennen väriä ja tekstiä

  // Tarkistetaan, mikä vastaus on ja muutetaan näytön väri sen mukaan
  if (body == "Logged in") {
    lcd.setRGB(0, 255, 0);  // Vihreä väri (Logged in)
    lcd.setCursor(0, 0);
    lcd.print("Logged in");
    tone(buzzerPin, 1200, 200);  // 1200 Hz taajuus 200 millisekunnin ajan
  } 
  else if (body == "Access denied" || "Room not found") {
    lcd.setRGB(255, 0, 0);  // Punainen väri (Access denied)
    lcd.setCursor(0, 0);
    lcd.print(body);
    tone(buzzerPin, 500, 200);  // 500 Hz taajuus 500 millisekunnin ajan
    delay(300);
    tone(buzzerPin, 500, 500);  // 500 Hz taajuus 500 millisekunnin ajan
    delay(300);
    tone(buzzerPin, 500, 500);  // 500 Hz taajuus 500 millisekunnin ajan
  } 
  else if (body == "Logged out") {
    lcd.setRGB(255, 255, 255);  // Valkoinen väri (Logged out)
    lcd.setCursor(0, 0);
    lcd.print("Logged out");
    tone(buzzerPin, 400, 300);  // 400 Hz taajuus 300 millisekunnin ajan
  }
} 


