#include "arduino_secrets.h"

#include "Firebase_Arduino_WiFiNINA.h"

#include <SPI.h>
#include <MFRC522.h>
#define SS_PIN 10
#define RST_PIN 9

// led indicators
#define GREEN_LED 2
#define RED_LED 3

// dummy inputs to simulate RFID cards
#define CARD_DUMMY1 4
#define CARD_DUMMY2 5
#define CARD_DUMMY3 A7 

// dummy ids corresponding to dummy cards
#define ID_DUMMY1 "AD3546F3"
#define ID_DUMMY2 "61AF3262"
#define ID_DUMMY3 "4123DA3C"

char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
int status = WL_IDLE_STATUS;

char db_url[] = SECRET_DB_URL;
char db_sec[] = SECRET_DB_SEC;

FirebaseData fbdo;
String path = "/";

MFRC522 rfid(SS_PIN, RST_PIN);

void setup()
{
    pinMode(GREEN_LED, OUTPUT);
    pinMode(RED_LED, OUTPUT);

    pinMode(CARD_DUMMY1, INPUT);
    pinMode(CARD_DUMMY2, INPUT);
    pinMode(CARD_DUMMY3, INPUT);

    Serial.begin(9600);
    SPI.begin();
    rfid.PCD_Init();

    // wait for serial port, only dev environment!
    while (!Serial)
        ;

    while (status != WL_CONNECTED)
    {
        Serial.print("Attempting to connect to network: ");
        Serial.println(ssid);
        status = WiFi.begin(ssid, pass);
        delay(10000);
    }

    Serial.println("You're connected to the network");

    Serial.println("---------------------------------------");
    Serial.println("Board Information:");
    // print your board's IP address:
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    // print your network's SSID:
    Serial.println();
    Serial.println("Network Information:");
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());
    Serial.println("---------------------------------------");

    // establish connection with firebase
    Firebase.begin(db_url, db_sec, ssid, pass);
    Firebase.reconnectWiFi(true);
}

void loop()
{
    getID();
    
    if (!digitalRead(CARD_DUMMY1)) {
      updateStatus(ID_DUMMY1);
    }

    if (!digitalRead(CARD_DUMMY2)) {
      updateStatus(ID_DUMMY2);
    }

    if (!digitalRead(CARD_DUMMY3)) {
      updateStatus(ID_DUMMY3);
    }
}

void getID()
{
    if (!rfid.PICC_IsNewCardPresent())
        return;
    if (!rfid.PICC_ReadCardSerial())
        return;

    String ID = "";
    for (byte i = 0; i < rfid.uid.size; i++)
    {
        ID.concat(String(rfid.uid.uidByte[i] < 0x10 ? " 0" : ""));
        ID.concat(String(rfid.uid.uidByte[i], HEX));
    }

    ID.toUpperCase();
    Serial.println(ID);

    updateStatus(ID);
    delay(1500);
}

void updateStatus(String ID)
{
  Serial.println(ID);
    if (Firebase.getBool(fbdo, path + "employees/" + ID + "/atWork"))
    {
        bool status = fbdo.boolData();

        if (Firebase.setBool(fbdo, path + "employees/" + ID + "/atWork", !status))
        {
            if (fbdo.boolData())
                digitalWrite(GREEN_LED, HIGH);
            else if (!fbdo.boolData())
                digitalWrite(RED_LED, HIGH);
            
            delay(2000);

            digitalWrite(GREEN_LED, LOW);
            digitalWrite(RED_LED, LOW);
        }
    }
    else
    {
        Serial.println(fbdo.errorReason());
    }
}
