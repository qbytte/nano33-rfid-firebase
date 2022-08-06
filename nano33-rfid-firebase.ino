// secrets file containing db and wifi credentials
#include "arduino_secrets.h"

// firebase library compatible with wifinina module
#include "Firebase_Arduino_WiFiNINA.h"

// rtc library
#include <RTCZero.h>

// rfid reader libraries and pins
#include <SPI.h>
#include <MFRC522.h>
#define SS_PIN 10
#define RST_PIN 9

// led indicators
#define YELLOW_LED 2
#define GREEN_LED 3
#define RED_LED 4

// wifi credentials obtained from secrets file
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
int status = WL_IDLE_STATUS;

// db credentials obtained from secrets file
char db_url[] = SECRET_DB_URL;
char db_sec[] = SECRET_DB_SEC;

// rtc library instance
RTCZero rtc;

// firebase library instance and root path of realtime db
FirebaseData fbdo;
String path = "/";

// rfid reader initialization
MFRC522 rfid(SS_PIN, RST_PIN);

void setup()
{
    // led indicators as outputs
    pinMode(YELLOW_LED, OUTPUT);
    pinMode(GREEN_LED, OUTPUT);
    pinMode(RED_LED, OUTPUT);

    // serial communication initialization
    Serial.begin(9600);

    // SPI communication initialization and rfid reader initialization
    SPI.begin();
    rfid.PCD_Init();

    // start rtc clock and set time and date
    rtc.begin();
    rtc.setTime(7, 0, 0);
    rtc.setDate(5, 8, 22);

    // wait for serial port, only dev environment!
    // while (!Serial);

    // wait for connection to network 
    while (status != WL_CONNECTED)
    {
        // waiting for connection indicator
        digitalWrite(YELLOW_LED, HIGH);
        
        Serial.print("Attempting to connect to network: ");
        Serial.println(ssid);
        status = WiFi.begin(ssid, pass);
        delay(10000);
        digitalWrite(YELLOW_LED, LOW);
    }

    // connection indicator
    for (int i = 0; i <= 3; i++) 
    {
      digitalWrite(GREEN_LED, HIGH);
      delay(350);
      digitalWrite(GREEN_LED, LOW);
      delay(350);
    }

    // print ip and network
    Serial.println("You're connected to the network");
    Serial.println("---------------------------------------");
    Serial.println("Board Information:");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
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
    // constantly wait for a card to be read
    getID();
}

/*
    getID() function
    gets the ID of the card being read in a string format and calls updateStatus()
    to change the status of the obtained ID
*/
void getID()
{
    // wait for card to be read
    if (!rfid.PICC_IsNewCardPresent())
        return;
    if (!rfid.PICC_ReadCardSerial())
        return;

    // through a for cycle the card ID is extracted
    String ID = "";
    for (byte i = 0; i < rfid.uid.size; i++)
    {
        ID.concat(String(rfid.uid.uidByte[i] < 0x10 ? " 0" : ""));
        ID.concat(String(rfid.uid.uidByte[i], HEX));
    }

    ID.toUpperCase();

    // call to update status to change the ID current status (working or not)
    updateStatus(ID);
}

/*
    updateStatus() function
    using the ID obtained through getID() the status of the employee assinged to specified ID gets updated between
    working or not, this function sends time and date as well
*/
void updateStatus(String ID)
{
    // led indicating a card has been read
    digitalWrite(YELLOW_LED, HIGH);
    delay(250);
    digitalWrite(YELLOW_LED, LOW);
    delay(250);

    // built endpoint for easier access to the employee data
    String endPoint = path + "employees/" + ID;
    Serial.println(endPoint);

    // current status of employee obtained (atWork = true || false)
    if (Firebase.getBool(fbdo, endPoint + "/atWork"))
    {
        // status saved in a variable
        bool status = fbdo.boolData();

        // update status to the opposite of the current one (if true => false : false => true)
        if (Firebase.setBool(fbdo, endPoint + "/atWork", !status))
        {
            // if the status updated is true, the employee is starting the shift
            if (fbdo.boolData())
            {
                // sending current time to db
                if (Firebase.setString(fbdo, endPoint + "/start", getTime()))
                    // logged in indicator
                    digitalWrite(GREEN_LED, HIGH);
                // log error reason
                else
                {
                    digitalWrite(YELLOW_LED, HIGH);
                    Serial.println(fbdo.errorReason());
                }
            }
            // if the status updated is false, the employee is ending the shift
            else if (!fbdo.boolData())
            {
                // sending current time and date to db
                if (Firebase.setString(fbdo, endPoint + "/finish", getTime()) && Firebase.setString(fbdo, endPoint + "/date", getDate()))
                {
                    // logged out indicator
                    digitalWrite(RED_LED, HIGH);
                    // log data at the end of the shift
                    logData(ID);
                }
                // log error reason
                else
                {
                    digitalWrite(YELLOW_LED, HIGH);
                    Serial.println(fbdo.errorReason());
                }
            }
        }
        // log error reason
        else
        {
            digitalWrite(YELLOW_LED, HIGH);
            Serial.println(fbdo.errorReason());
        }
    }
    // log error reason
    else
    {
        digitalWrite(YELLOW_LED, HIGH);
        Serial.println("Path does not exist");
    }
    // delay to avoid multiple petitions
    delay(1000);

    // turning off led indicators
    digitalWrite(YELLOW_LED, LOW);
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(RED_LED, LOW);
}

/*
    logData() function
    this function first retrieves specific data from the realtime db...
        - ID (already obtained from card read)
        - start hr
        - finish hr
        - date
    
    once all data is retrieved it is parsed into a string, then the string is pushed
    to the logs section of the remote db
*/
void logData(String ID) 
{
    // built endpoint for easier access to the employee data
    String endPoint = path + "employees/" + ID;

    // commence building a json string with data obtained from db
    String json = "{\"id\":\"" + ID;

    if (Firebase.getString(fbdo, endPoint + "/start"))
    {
        json += "\", \"start\":\"" + fbdo.stringData();
        if (Firebase.getString(fbdo, endPoint + "/finish"))
        {
            json += "\", \"finish\":\"" + fbdo.stringData();
            if (Firebase.getString(fbdo, endPoint + "/date"))
            {
                json += "\", \"date\":\"" + fbdo.stringData() + "\"}";
            }
            else Serial.println(fbdo.errorReason());
        }
        else Serial.println(fbdo.errorReason());
    }
    else Serial.println(fbdo.errorReason());

    // json string built
    Serial.print("Data to be sent: ");
    Serial.println(json);

    if (Firebase.pushJSON(fbdo, path + "logs/", json)) 
    {
        // database path to be appended
        Serial.print("Data path appended: ");
        Serial.println(fbdo.dataPath());

        // new created key/name
        Serial.print("Log ID: ");
        Serial.println(fbdo.pushName());

        Serial.print("Data pushed successfully");
    }
    else Serial.println(fbdo.errorReason());
}

/*
    getTime() function
    using rtc library, time's obtained and put into a string format
*/
String getTime() 
{
    // get hrs, mins and secs
    int h = rtc.getHours();
    int m = rtc.getMinutes();
    int s = rtc.getSeconds();

    String time = "";

    // if any digit's less than zero add a zero to the left to have a standard format
    if (h < 10) time += "0";
    time += h;
    time += ":";
    if (m < 10) time += "0";
    time += m;
    time += ":";
    if (s < 10) time += "0";
    time += s;

    // return string with current hour
    return time;
}

/*
    getDate() function
    using rtc library, date's obtained and put into a string format
*/
String getDate()
{
    // get day, month and year
    int d = rtc.getDay();
    int mo = rtc.getMonth();
    int yr = rtc.getYear();

    String date = "";

    date += "20";

    // if any digit's less than zero add a zero to the left to have a standard format
    if (yr < 10) date += "0";
    date += yr;
    date += "-";
    if (mo < 10) date += "0";
    date += mo;
    date += "-";
    if (d < 10) date += "0";
    date += d;

    // return string with current date
    return date;
}
