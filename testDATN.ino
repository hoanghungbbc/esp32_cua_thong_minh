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
#include <WiFiManager.h>
#include <Keypad.h>
#include <Adafruit_Fingerprint.h>

#define BUZZER 2
#define SS_PIN 5
#define RST_PIN 4
#define SERVO_PIN 13

MFRC522 rfid(SS_PIN, RST_PIN);
Servo myservo;
Preferences preferences;
LiquidCrystal_I2C lcd(0x27,16,2);
WiFiManager wm;

String wifiPass = "AAAA";

String cardUID[20];
int cardID[20];   // thêm mảng ID
int cardCount;
//int nextCardID = 1;

int nextFingerID = 1;

bool addMode = false;
bool deleteMode = false;
bool enterDeleteID = false;
String deleteIDInput = "";
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

/* ---------- FUNCTION DECLARE ---------- */

void beep(int timeDelay);
bool checkWiFiPassword();
void resetWiFi();

/* ---------- BLYNK ---------- */

BLYNK_WRITE(V3)
{
  int v = param.asInt();

  if(v == 1)
  {
    enterDeleteID = true;
    deleteIDInput = "";

    lcd.clear();
    lcd.print("Nhap ID xoa");
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
    preferences.putString(("uid"+String(i)).c_str(), cardUID[i]);
    preferences.putInt(("id"+String(i)).c_str(), cardID[i]);
  }
}
void deleteCardByID(int id)
{
  for(int i=0;i<cardCount;i++)
  {
    if(cardID[i] == id)
    {
      for(int j=i;j<cardCount-1;j++)
      {
        cardUID[j] = cardUID[j+1];
        cardID[j]  = cardID[j+1];
      }

      cardCount--;

      saveCards();

      lcd.clear();
      lcd.print("Da xoa ID:");
      lcd.print(id);

      delay(2000);
      return;
    }
  }

  lcd.clear();
  lcd.print("Khong tim thay");
  delay(2000);
}
void loadCards()
{
  cardCount = preferences.getInt("count",0);

  for(int i=0;i<cardCount;i++)
  {
    cardUID[i] = preferences.getString(("uid"+String(i)).c_str(),"");
    cardID[i]  = preferences.getInt(("id"+String(i)).c_str(),0);
  }
}


int getAvailableID()
{
  for(int id = 1; id <= 20; id++)
  {
    bool used = false;

    for(int i = 0; i < cardCount; i++)
    {
      if(cardID[i] == id)
      {
        used = true;
        break;
      }
    }

    if(!used)
    {
      return id;
    }
  }

  return -1;
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

    nextFingerID = 1;

    saveFingerID();

    delay(1000);
  }
  else
  {
    lcd.clear();
    lcd.print("Xoa that bai");

    delay(1000);
  }
}

void addFingerprint()
{
  int id = nextFingerID;

  lcd.clear();
  lcd.print("ID moi:");

  lcd.setCursor(0,1);
  lcd.print(id);

  delay(1000);

  lcd.clear();
  lcd.print("Dat van tay");

  while(finger.getImage()!=FINGERPRINT_OK);

  finger.image2Tz(1);

  lcd.clear();
  lcd.print("Dat lan 2");

  delay(1000);

  while(finger.getImage()!=FINGERPRINT_OK);

  finger.image2Tz(2);

  if(finger.createModel()==FINGERPRINT_OK)
  {
    finger.storeModel(id);

    lcd.clear();
    lcd.print("Da them ID:");

    lcd.setCursor(0,1);
    lcd.print(id);

    nextFingerID++;

    saveFingerID();

    delay(2000);
  }
  else
  {
    lcd.clear();
    lcd.print("Them that bai");

    delay(1000);
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

        delay(1000);

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
  if(finger.getImage()!=FINGERPRINT_OK)
  {
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

    beep(200);
    delay(200);

    beep(200);

    lcd.clear();
    lcd.print("Dong cua");

    delay(1000);
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
          return true;
        }
        else
        {
          lcd.clear();
          lcd.print("Sai pass");
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

  // ===== MỞ CỬA BẰNG MẬT KHẨU =====
  if(key=='D')
  {
    if(checkPassword())
    {
      lcd.clear();
      lcd.print("Mo bang MK");
      openDoor();
    }
    else
    {
      lcd.clear();
      lcd.print("Sai MK");

      beep(200);
      delay(200);
      beep(200);
    }
  }

  // ===== ĐỔI MẬT KHẨU =====
  if(key=='#')
  {
    changePassword();
  }

  if(key=='*')
  {
    if(checkPassword())
    {
      addFingerprint();
    }
  }

  if(key=='C')
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

  if(key=='A')
  {
    if(checkWiFiPassword())
    {
      resetWiFi();
    }
  }
}

/* ---------- Buzzer ---------- */

void beep(int timeDelay)
{
  digitalWrite(BUZZER, HIGH);

  delay(timeDelay);

  digitalWrite(BUZZER, LOW);
}

/* ---------- WiFi ---------- */

bool checkWiFiPassword()
{
  lcd.clear();
  lcd.print("Pass WiFi:");

  String input="";

  while(true)
  {
    char key = keypad.getKey();

    if(key)
    {
      if(key=='#')
      {
        if(input == wifiPass)
        {
          lcd.clear();
          lcd.print("Dung MK");

          delay(500);

          return true;
        }
        else
        {
          lcd.clear();
          lcd.print("Sai MK");

          delay(500);

          return false;
        }
      }
      else
      {
        input += key;

        lcd.setCursor(0,1);
        lcd.print(input);
      }
    }
  }
}

void resetWiFi()
{
  lcd.clear();
  lcd.print("Reset WiFi");

  wm.resetSettings();

  delay(1000);

  lcd.clear();
  lcd.print("Nhap WiFi moi");

  wm.startConfigPortal("ESP32_KhoaCua");

  lcd.clear();
  lcd.print("Done!");

  delay(500);

  ESP.restart();
}

/* ---------- SETUP ---------- */
void savePassword(String newPass)
{
  preferences.putString("pass", newPass);
}
void changePassword()
{
  lcd.clear();
  lcd.print("Pass cu:");

  String oldPass="";

  // nhập mật khẩu cũ
  while(true)
  {
    char k = keypad.getKey();

    if(k)
    {
      if(k=='#')
      {
        if(oldPass == password)
        {
          break;
        }
        else
        {
          lcd.clear();
          lcd.print("Sai pass");
          delay(1000);
          return;
        }
      }
      else
      {
        oldPass += k;
        lcd.setCursor(0,1);
        lcd.print(oldPass);
      }
    }
  }

  // nhập mật khẩu mới
  lcd.clear();
  lcd.print("Pass moi:");

  String newPass="";

  while(true)
  {
    char k = keypad.getKey();

    if(k)
    {
      if(k=='#')
      {
        password = newPass;
        savePassword(newPass);

        lcd.clear();
        lcd.print("Da doi MK");

        delay(1000);
        return;
      }
      else
      {
        newPass += k;
        lcd.setCursor(0,1);
        lcd.print(newPass);
      }
    }
  }
}
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
password = preferences.getString("pass", "1111");
  loadCards();
  loadFingerID();

  Wire.begin(21,22);

  lcd.init();
  lcd.backlight();

  lcd.print("System Ready");

  mySerial.begin(57600,SERIAL_8N1,16,17);

  finger.begin(57600);

  bool res = wm.autoConnect("ESP32_KhoaCua");

  if(!res)
  {
    lcd.clear();
    lcd.print("Config WiFi");

    wm.startConfigPortal("ESP32_KhoaCua");
  }

  lcd.clear();
  lcd.print("WiFi OK");

  Blynk.begin(BLYNK_AUTH_TOKEN,
              WiFi.SSID().c_str(),
              WiFi.psk().c_str());
}

/* ---------- LOOP ---------- */

void loop()
{
  Blynk.run();

  keypadControl();

  checkFingerprint();

if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial())
{
  String uid="";

for(byte i=0;i<rfid.uid.size;i++)
{
  if(rfid.uid.uidByte[i] < 0x10)
  {
    uid += "0";
  }

  uid += String(rfid.uid.uidByte[i], HEX);
}

  uid.toUpperCase();

  if(addMode)
  {
    if(checkCard(uid))
    {
      lcd.clear();
      lcd.print("The da ton tai");
      delay(1000);
    }
    else
    {
      if(cardCount < 20)
      {
        cardUID[cardCount] = uid;
int newID = getAvailableID();

if(newID == -1)
{
  lcd.clear();
  lcd.print("Het ID");
  return;
}

cardID[cardCount] = newID;

lcd.clear();
lcd.print("ID:");
lcd.print(newID);

cardCount++;

        saveCards();
        delay(1000);
      }
      else
      {
        lcd.clear();
        lcd.print("Full bo nho");
      }
    }

    addMode = false;
  }
  else
  {
    if(checkCard(uid))
    {
      for(int i=0;i<cardCount;i++)
      {
        if(cardUID[i] == uid)
        {
          lcd.clear();
          lcd.print("ID:");
          lcd.print(cardID[i]);
          delay(500);
          break;
        }
      }

      openDoor();
    }
    else
    {
      lcd.clear();
      lcd.print("The khong hop le");
      delay(1500);

      lcd.clear();
      lcd.print("Dong cua");
      delay(500);
    }
  }

  rfid.PICC_HaltA();
}
  if(enterDeleteID)
{
  char k = keypad.getKey();

  if(k)
  {
    if(k=='#')
    {
      deleteCardByID(deleteIDInput.toInt());
      enterDeleteID = false;
    }
    else
    {
      deleteIDInput += k;
      lcd.setCursor(0,1);
      lcd.print(deleteIDInput);
    }
  }
}


}
