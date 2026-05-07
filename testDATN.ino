#define BLYNK_TEMPLATE_ID "TMPL6pyoCO7pG"
#define BLYNK_TEMPLATE_NAME "cuoi"
#define BLYNK_AUTH_TOKEN "yvK1icM3IgQdId3KdS0nljPdnjJc1TQD"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>
#include <Preferences.h>

#include <Keypad.h>
#include <Adafruit_Fingerprint.h>

char ssid[] = "hu.g";
char pass[] = "135792468";
#define BUZZER 2
#define SS_PIN 5
#define RST_PIN 4
#define SERVO_PIN 13

MFRC522 rfid(SS_PIN, RST_PIN);
Servo myservo;
Preferences preferences;

LiquidCrystal_I2C lcd(0x27,16,2);

String cardUID[20];
int cardCount = 0;
int nextFingerID = 1;
bool addMode = false;
bool deleteMode = false;

/* ------------ Fingerprint ------------- */

HardwareSerial mySerial(2);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

/* ------------ Keypad ------------------ */

const byte ROWS = 4;
const byte COLS = 4;

char keys[ROWS][COLS] =
{
{'1','2','3','A'},
{'4','5','6','B'},
{'7','8','9','C'},
{'*','0','#','D'}
};

byte rowPins[ROWS] = {32,33,25,26};
byte colPins[COLS] = {27,14,12,15};

Keypad keypad = Keypad(makeKeymap(keys),rowPins,colPins,ROWS,COLS);

String password="1111";
String input="";

/* ---------- BLYNK ---------- */

BLYNK_WRITE(V3)
{
int v = param.asInt();
if(v == 1)
{
deleteMode = true;
lcd.clear();
lcd.print("Quet the xoa");
}
}

BLYNK_WRITE(V0)
{
int value = param.asInt();

if(value == 1)
{
myservo.write(90);
lcd.clear();
lcd.print("mo cua");
}
else
{
myservo.write(0);
lcd.clear();
lcd.print("dong cua");
}
}

BLYNK_WRITE(V2)
{
int v = param.asInt();

if(v == 1)
{
addMode = true;
lcd.clear();
lcd.print("them the moi");
}
}

/* ---------- RFID ---------- */

bool checkCard(String uid)
{
for(int i=0;i<cardCount;i++)
{
if(uid == cardUID[i]) return true;
}
return false;
}

void saveCards()
{
preferences.putInt("count", cardCount);

for(int i=0;i<cardCount;i++)
{
String key = "card" + String(i);
preferences.putString(key.c_str(), cardUID[i]);
}
}

void loadCards()
{
cardCount = preferences.getInt("count",0);

for(int i=0;i<cardCount;i++)
{
String key = "card" + String(i);
cardUID[i] = preferences.getString(key.c_str(),"");
}
}

void deleteCard(String uid)
{
for(int i=0;i<cardCount;i++)
{
if(cardUID[i] == uid)
{
for(int j=i;j<cardCount-1;j++)
{
cardUID[j] = cardUID[j+1];
}

cardCount--;
saveCards();

lcd.clear();
lcd.print("Da xoa the");
return;
}
}

lcd.clear();
lcd.print("Khong tim thay");
}
void saveFingerID()
{
  preferences.putInt("fid", nextFingerID);
}

void loadFingerID()
{
  nextFingerID = preferences.getInt("fid",1);
}
/* ---------- Servo ---------- */

void openDoor()
{
myservo.write(90);
lcd.clear();
lcd.print("Mo cua");
delay(3000);
myservo.write(0);
lcd.clear();
lcd.print("Dong cua");
}

/* ---------- Fingerprint ---------- */
void deleteAllFingerprint()
{
lcd.clear();
lcd.print("Dang xoa...");

uint8_t p = finger.emptyDatabase();

if(p == FINGERPRINT_OK)
{
lcd.clear();
lcd.print("Da xoa tat ca");

nextFingerID = 1;     // reset ID vân tay
saveFingerID();       // lưu lại

delay(2000);
}
else
{
lcd.clear();
lcd.print("Xoa that bai");
delay(2000);
}
}
void addFingerprint()
{
int id = nextFingerID;

lcd.clear();
lcd.print("ID moi:");
lcd.setCursor(0,1);
lcd.print(id);

delay(2000);

lcd.clear();
lcd.print("Dat van tay");

while(finger.getImage()!=FINGERPRINT_OK);

finger.image2Tz(1);

lcd.clear();
lcd.print("Dat lan 2");

delay(2000);

while(finger.getImage()!=FINGERPRINT_OK);

finger.image2Tz(2);

if(finger.createModel()==FINGERPRINT_OK)
{
finger.storeModel(id);

lcd.clear();
lcd.print("Da them ID:");
lcd.setCursor(0,1);
lcd.print(id);

nextFingerID++;     // tăng ID
saveFingerID();     // lưu lại

delay(2000);
}
else
{
lcd.clear();
lcd.print("Them that bai");
delay(2000);
}
}

void deleteFingerprint()
{
lcd.clear();
lcd.print("Nhap ID xoa");

String id="";

while(true)
{
char k=keypad.getKey();

if(k)
{
if(k=='#')
{
finger.deleteModel(id.toInt());

lcd.clear();
lcd.print("Da xoa VT");
delay(2000);
break;
}
else
{
id+=k;
lcd.setCursor(0,1);
lcd.print(id);
}
}
}
}

void checkFingerprint()
{
if(finger.getImage()!=FINGERPRINT_OK){
  return;
} 

finger.image2Tz();

if(finger.fingerFastSearch()==FINGERPRINT_OK)
{
lcd.clear();
lcd.print("Van tay dung ID:");
lcd.setCursor(0,1);
lcd.print(finger.fingerID);
delay(500);
openDoor();
}
else
{
lcd.clear();
lcd.print("VT sai");

beep(200);
delay(200);
beep(200);   // beep 3 lần báo sai
delay(200);
beep(200);
lcd.clear();
lcd.print("Dong cua");
delay(1500);
}
}

/* ---------- Keypad ---------- */

bool checkPassword()
{
lcd.clear();
lcd.print("Nhap pass");

input="";

while(true)
{
char key = keypad.getKey();

if(key)
{
if(key=='#')
{
if(input==password)
{
lcd.clear();
lcd.print("Pass dung");
delay(1000);
return true;
}
else
{
lcd.clear();
lcd.print("Sai pass");
delay(1500);
return false;
}
}
else
{
input+=key;
lcd.setCursor(0,1);
lcd.print(input);
}
}
}
}

void keypadControl()
{
char key = keypad.getKey();

if(key=='*')
{
if(checkPassword())
{
addFingerprint();
}
}
if(key=='C')     // xoa tat ca van tay
{
if(checkPassword())
{
deleteAllFingerprint();
}
}
if(key=='B')
{
if(checkPassword())
{
deleteFingerprint();
}
}
}
void beep(int timeDelay)
{
digitalWrite(BUZZER, HIGH);
delay(timeDelay);
digitalWrite(BUZZER, LOW);
}
/* ---------- SETUP ---------- */

void setup()
{
  pinMode(BUZZER, OUTPUT);
digitalWrite(BUZZER, LOW);
Serial.begin(115200);

SPI.begin();
rfid.PCD_Init();

myservo.attach(SERVO_PIN);
myservo.write(0);

preferences.begin("rfid", false);
loadCards();

Wire.begin(21,22);

lcd.init();
lcd.backlight();

lcd.print("System Ready");

mySerial.begin(57600,SERIAL_8N1,16,17);
finger.begin(57600);

Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
}

/* ---------- LOOP ---------- */

void loop()
{
Blynk.run();

keypadControl();
checkFingerprint();

if (!rfid.PICC_IsNewCardPresent()) return;
if (!rfid.PICC_ReadCardSerial()) return;

String uid="";

for(byte i=0;i<rfid.uid.size;i++)
{
uid += String(rfid.uid.uidByte[i],HEX);
}

if(addMode)
{
if(checkCard(uid))   // đã tồn tại
{
  lcd.clear();
  lcd.print("The da ton tai");
  delay(1500);
}
else
{
  if(cardCount < 3)//tran bo nho----------
  {
    cardUID[cardCount++] = uid;
    saveCards();

    lcd.clear();
    lcd.print("Da them the");
  }
  else
  {
    lcd.clear();
    lcd.print("Full bo nho");
  }
}

addMode = false;
}

else if(deleteMode)
{
deleteCard(uid);
deleteMode=false;
}

else
{
if(checkCard(uid))
{
openDoor();
}
else
{
lcd.clear();
lcd.print("The khong hop le");
delay(1500);
lcd.clear();
lcd.print("Dong cua");
delay(1500);
}
}

rfid.PICC_HaltA();
}