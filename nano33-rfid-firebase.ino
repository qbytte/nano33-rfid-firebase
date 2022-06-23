#include "arduino_secrets.h"

#include "Firebase_Arduino_WiFiNINA.h"

#include <SPI.h>
#include <MFRC522.h>
#define SS_PIN 10
#define RST_PIN 9

#define RED_LED 3
#define GREEN_LED 4

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
    pinMode(RED_LED, OUTPUT);
    pinMode(GREEN_LED, OUTPUT);

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
}