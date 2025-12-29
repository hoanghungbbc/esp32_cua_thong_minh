#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <U8g2lib.h> 
#include <Preferences.h>
#include <HardwareSerial.h> //dung de giao tiep usar
#include <Adafruit_Fingerprint.h> 
#include <ArduinoJson.h> // thu vien de nhan Json
#include "time.h"
int timekhoa=0;
unsigned long lastBeep = 0;
bool buzzerState = false;
bool fireDetected = false;
const int ledChinh = 2;      // LED chính: sáng tắt liên tục trong loop()
#define khoa 18                    
#define xac_nhan 34
#define chuyen_tiep 35
#define tien 33
#define lui 32

#define loa  26

#define FLAME_DIGITAL 14
// Cảm biến gas
#define GAS_DIGITAL   13
#define GAS_ANALOG    39

  int flameAnalog = 0;
  int gasAnalog   = 0;

  int flameDigital = 0;
  int gasDigital   = 0;

// Khởi tạo U8g2 cho SSD1306 128x64 qua I2C (ESP32)
U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// Khai báo UART2 cho ESP32 (TX: 17, RX: 16)
HardwareSerial van_tay_Serial(2);
Adafruit_Fingerprint van_tay = Adafruit_Fingerprint(&van_tay_Serial);
// ================= WIFI =================
Preferences prefs;
String ssidSaved, passSaved;
/* ================== NTP (GIỜ VIỆT NAM) ================== */
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 7 * 3600;
const int daylightOffset_sec = 0;

/* ================== HAM LAY THOI GIAN ================== */
String getDateTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "UNKNOWN";
  }
  char buffer[25];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
  return String(buffer);
}
// ================= CHARSET =================
char charset[] =
" ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"abcdefghijklmnopqrstuvwxyz"
"0123456789";
int charIndex = 0;
int menuIndex = 0;
String menuItems[3] = {
  "Ket noi WiFi",
  "Nhap WiFi moi",
  "Reset WiFi"
};
// HiveMQ Cloud
const char* mqtt_server = "5f14affda214460285ae6e28e8a5727b.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* mqtt_user = "Duong";
const char* mqtt_pass = "Duong123";
// SSL client
WiFiClientSecure espClient;
PubSubClient client(espClient);
int quet_van_tay();
void xoa_van_tay_toan_bo();
void mo_khoa();
void themvantay(int id);
void hienthi_oled(char *m , int x , int y );
void loakeu(){
  digitalWrite(loa, HIGH);
  delay(100);  // kêu "tít"
  digitalWrite(loa, LOW);
  delay(100);  // ngắt
  digitalWrite(loa, HIGH);
  delay(100);  // kêu "tít"
  digitalWrite(loa, LOW);
}
//Hàm này tự động được gọi mỗi khi esp nhận được một message từ MQTT broker.
void callback(char* topic, byte* message, unsigned int length) {
  String msg;
  for (int i = 0; i < length; i++) msg += (char)message[i];
  Serial.println("JSON nhận: " + msg);  
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, msg);
  if (error) {
    Serial.println(" Lỗi JSON");
    return;
  }

  if (doc.containsKey("khoa")) {
    String val = doc["khoa"];
   // digitalWrite(ledChinh, val == "ON" ? HIGH : LOW);
    if(val=="mo")  mo_khoa();
  }

   else if (doc.containsKey("iddk") ) {
    //digitalWrite(fanPin, (int)doc["fan"] == 1 ? HIGH : LOW);
     int id=(int)doc["iddk"];
     
     themvantay(id);
 }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Kết nối MQTT...");
    if (client.connect("ESP32Client", mqtt_user, mqtt_pass)) {
      Serial.println("✅ Thành công");
      client.subscribe("esp32/led");
    } else {
      Serial.print("❌ Lỗi = ");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

// =================================================
void oledShow(String l1, String l2="", String l3="") {
  display.clearBuffer();
  display.setFont(u8g2_font_ncenB08_tr); // font nhỏ gọn
  display.setCursor(0, 12);
  display.print(l1);
  display.setCursor(0, 26);
  display.print(l2);
  display.setCursor(0, 40);
  display.print(l3);
  display.sendBuffer();
}

// =================================================
void loadWiFi() {
  prefs.begin("wifi", true);
  ssidSaved = prefs.getString("ssid", "");
  passSaved = prefs.getString("pass", "");
  prefs.end();
}

// =================================================
void saveWiFi(String ssid, String pass) {
  prefs.begin("wifi", false);
  prefs.putString("ssid", ssid);
  prefs.putString("pass", pass);
  prefs.end();
}

// =================================================
bool connectWiFi(String ssid, String pass) {
  WiFi.begin(ssid.c_str(), pass.c_str());
  unsigned long t = millis();
  while (WiFi.status() != WL_CONNECTED) {
    oledShow("Dang ket noi:", ssid, "Cho...");
    if (millis() - t > 15000) {
      WiFi.disconnect(true);
      return false;
    }
    delay(500);
  }
  return true;
}
// NHẬP CHUỖI BẰNG OLED + NÚT
String inputString(String title) {
  String text = "";
  charIndex = 0;
  unsigned long lastBlink = 0;
  bool showChar = true;

  while (1) {
    if (millis() - lastBlink > 500) {
      showChar = !showChar;
      lastBlink = millis();
    }

    display.clearBuffer();
    display.setFont(u8g2_font_ncenB08_tr);
    display.setCursor(0, 12);
    display.print(title);
    display.setCursor(0, 28);
    display.print(text);
    if (showChar) display.print(charset[charIndex]);
    else display.print("_");
    display.setCursor(0, 50);
    display.print("UP/DOWN NEXT OK");
    display.sendBuffer();

    if (!digitalRead(tien)) {
      charIndex = (charIndex + 1) % (sizeof(charset) - 1);
      delay(200);
    }
    if (!digitalRead(lui)) {
      charIndex--;
      if (charIndex < 0) charIndex = sizeof(charset) - 2;
      delay(200);
    }
    if (!digitalRead(chuyen_tiep)) {
      text += charset[charIndex];
      delay(200);
    }
    if (!digitalRead(xac_nhan)) {
      delay(300);
      return text;
    }
  }
}
// ===========================================
void showMenu() {
  display.clearBuffer();
  display.setFont(u8g2_font_ncenB08_tr);
  for (int i = 0; i < 3; i++) {
    display.setCursor(0, 12 + i*16);
    if (i == menuIndex) display.print("> ");
    else display.print("  ");
    display.print(menuItems[i]);
  }
  display.sendBuffer();
}

// =================================================
void resetWiFi() {
  prefs.begin("wifi", false);
  prefs.clear();          // xóa WiFi cũ
  prefs.end();
  WiFi.disconnect(true);

  oledShow("RESET WIFI", "Nhap WiFi moi");
  delay(2000);
  inputNewWiFi();
}

// =================================================
void inputNewWiFi() {
  String ssid = inputString("Nhap Ten WIFI:");
  String pass = inputString("Nhap mat khau:");

  if (connectWiFi(ssid, pass)) {
    oledShow("Thanh cong!", "Da luu WiFi");
    saveWiFi(ssid, pass);
  } else {
    oledShow("That bai!", "Nhap lai WiFi");
    delay(2000);
    inputNewWiFi();
  }
}

// =================================================
void handleMenu() {
  while (1) {
    showMenu();
    if (!digitalRead(tien)) {
      menuIndex--;
      if (menuIndex < 0) menuIndex = 2;
      delay(200);
    }
    if (!digitalRead(lui)) {
      menuIndex++;
      if (menuIndex > 2) menuIndex = 0;
      delay(200);
    }
    if (!digitalRead(xac_nhan)) {
      delay(300);
      switch (menuIndex) {
        case 0:
          if (ssidSaved != "") {
            oledShow("Dang ket noi", ssidSaved);
            if (connectWiFi(ssidSaved, passSaved)) {
              oledShow("Da ket noi!", WiFi.localIP().toString());
              return;
            }
          }
          oledShow("WiFi loi", "Nhap WiFi moi");
          delay(1500);
          inputNewWiFi();
          return;
        case 1:
          inputNewWiFi();
          return;
        case 2:
          resetWiFi();
          return;
      }
    }
  }
}

void setup() {
    Serial.begin(9600);
    pinMode(chuyen_tiep,INPUT);
    pinMode(xac_nhan,INPUT);
    pinMode(tien,INPUT);
    pinMode(lui,INPUT);
    pinMode(ledChinh, OUTPUT);
    pinMode(khoa,OUTPUT);
    pinMode(loa,OUTPUT);
    pinMode(FLAME_DIGITAL, INPUT);
    pinMode(GAS_DIGITAL, INPUT);
    //OLED
    // Khởi động màn hình
     display.begin();
     display.clearBuffer(); 
     hienthi_oled("ESP32 SAN SANG",10,20);
     display.sendBuffer();

     delay(2000); 
    //wifi
     loadWiFi();
     handleMenu();

  // KET NOI VAN TAY
     van_tay_Serial.begin(57600, SERIAL_8N1, 16, 17); // Kết nối      UART2 với AS608
     display.clearBuffer();
     hienthi_oled("-Dang kiem tra ket",0,12);
      hienthi_oled("noi van tay",0,24);
     display.sendBuffer(); 
     delay(2000);  

    if (van_tay.verifyPassword()) {
      display.clearBuffer();
      hienthi_oled("Ket noi thanh cong",0,20);
      display.sendBuffer(); 
    
      
    delay(2000);  

    } else {    
        display.clearBuffer();
        hienthi_oled("Ket noi",0,12);
        hienthi_oled(" khong thanh cong ",0,24);
        display.sendBuffer();
    
        delay(2000);
        display.clearBuffer();
        hienthi_oled("DUNG CHUONG TRINH",0,20);
        display.sendBuffer();
      while (1); // Dừng chương trình nếu không tìm thấy cảm biến
    }
      // ---- TIME ---- //
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  
      display.clearBuffer();
      hienthi_oled("Dong bo thoi gian",0,20);
      display.sendBuffer();
      delay(2000);

  espClient.setInsecure(); // Bỏ kiểm tra chứng chỉ (đơn giản cho thử nghiệm)
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}
unsigned long lastSend = 0;
void loop() {
      if(WiFi.status() !=WL_CONNECTED)
      {
             while(WiFi.status() !=WL_CONNECTED)
              {
                if (ssidSaved != "") {
                display.clearBuffer();
                oledShow("Dang ket noi", ssidSaved);
                display.sendBuffer();
                delay(2000);

                if (connectWiFi(ssidSaved, passSaved)) {
                  display.clearBuffer();
                oledShow("Da ket noi!", WiFi.localIP().toString());
                display.sendBuffer();
                delay(2000);
            }
          } 
        }
      }

      if (!van_tay.verifyPassword())
         while (!van_tay.verifyPassword())
           ketnoilaivantay();
       

      if(quet_van_tay()!=-1)  mo_khoa();


      

      int lua = digitalRead(FLAME_DIGITAL);
      int gasAnalog   = analogRead(GAS_ANALOG);
      
      

      struct tm timeinfo;
      getLocalTime(&timeinfo);

      char timeStr[20];
      strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);

      display.clearBuffer();
      display.setFont(u8g2_font_6x12_tf);

      display.setCursor(0, 12);
      display.print("Time: ");
      display.print(timeStr);

      display.setCursor(0, 28);
      display.print("cam bien lua: ");
      display.print(lua);
      display.setCursor(0, 52);
      display.print("cam bien khi gas:");
      display.print(gasAnalog);

      display.sendBuffer();

      // Ngưỡng cảnh báo (tự chỉnh)
      if ( gasAnalog > 3000) {
        digitalWrite(ledChinh, HIGH);   // Nguy hiểm
      } else {
        digitalWrite(ledChinh, LOW);    // An toàn
      }
        if (digitalRead(FLAME_DIGITAL) == LOW || gasAnalog > 3000) { 
          
          fireDetected = true;
        } else {
          fireDetected = false;
        }

      delay(600);
      if (!client.connected()) reconnect();
      client.loop();

        unsigned long now = millis();
        if (now - lastSend > 3000) { // gửi mỗi 3 giây
        char payload[100];
      snprintf(payload, sizeof(payload),
              "{\"flame\":%d,\"gas\":%d}",
              lua, gasAnalog);

      client.publish("esp32/sensor", payload);
        lastSend = now;
      }
  if (fireDetected) {
    if (millis() - lastBeep >= 300) { // 300ms đổi trạng thái
    lastBeep = millis();
     
    digitalWrite(loa, 1);
    delay(200);
    digitalWrite(loa, 0);
      digitalWrite(loa, 1);
    delay(200);
    digitalWrite(loa, 0);
  }
}
  }
//  Hàm lấy ID vân tay khi quet
int quet_van_tay() {
  uint8_t p = van_tay.getImage();     // Bước 1: Lấy ảnh vân tay
  if (p != FINGERPRINT_OK) return -1;       // Nếu không thành công, trả về -1
  p = van_tay.image2Tz();               // Bước 2: Chuyển ảnh thành template
  if (p != FINGERPRINT_OK) return -1;
  p = van_tay.fingerFastSearch();      // Bước 3: So sánh với các template đã lưu
  if (p != FINGERPRINT_OK) return -1;
  return van_tay.fingerID;                  // Nếu thành công, trả về ID của vân tay
}
//  Hàm đăng ký vân tay mới
void dang_ky_van_tay(int id) {
      
        display.clearBuffer();
        hienthi_oled("bat dau dang ky van tay",0,10);
        display.sendBuffer(); 

        delay(1000);
   
        display.clearBuffer();
        hienthi_oled("Dat ngon tay len cam bien",0,10);
        display.sendBuffer(); 

      
  while (van_tay.getImage() != FINGERPRINT_OK);
  if (van_tay.image2Tz(1) != FINGERPRINT_OK) {

        display.clearBuffer();
        hienthi_oled("Loi khi lay van tay",0,10);
        display.sendBuffer(); 

    return;
  }
        display.clearBuffer();
        hienthi_oled("Lan 1 thanh cong",0,10);
        display.sendBuffer(); 
        
        delay(2000);

        display.clearBuffer();
        hienthi_oled("Nhac tay va ",0,10);
        hienthi_oled("dat lai",0,20);
        display.sendBuffer(); 
       delay(1000);
  while (van_tay.getImage() != FINGERPRINT_NOFINGER);
       display.clearBuffer();
        hienthi_oled("Dat lai ngon tay",0,20);
        display.sendBuffer(); 
  while (van_tay.getImage() != FINGERPRINT_OK);
  if (van_tay.image2Tz(2) != FINGERPRINT_OK) {
        display.clearBuffer();
        hienthi_oled("Loi khi lay van tay",0,10);
        display.sendBuffer();
  }
  if (van_tay.createModel() != FINGERPRINT_OK) {
        display.clearBuffer();
        hienthi_oled("Khong the lay mau",0,10);
        display.sendBuffer();
    return;
  }
  if (van_tay.storeModel(id) != FINGERPRINT_OK) {
        display.clearBuffer();
        hienthi_oled("Loi khi luu mau",0,10);
        display.sendBuffer();
    return;
  }
        display.clearBuffer();
        hienthi_oled("Dang ky thanh cong",0,10);
        display.sendBuffer();
      delay(5000);
}
//  Hàm xóa toàn bộ vân tay
void xoa_van_tay_toan_bo() {

    if (van_tay.emptyDatabase() == FINGERPRINT_OK) {
 
        display.clearBuffer();
       
      hienthi_oled("Da xoa ", 40, 0);
      hienthi_oled("toan bo van tay", 10, 10);
      display.sendBuffer();   
      delay(1000);
    } else {
      display.clearBuffer();
      hienthi_oled("Loi khi xoa van tay", 0, 0);
      display.sendBuffer(); 
   
      delay(100);
      return;
      delay(1000);
    }
} 
//  Hàm xóa vân tay theo ID
void xoa_van_tay_theo_id(int id) {
      display.clearBuffer();  
      hienthi_oled("Dang xoa !",40,10);
      display.sendBuffer(); 
  if (van_tay.deleteModel(id) == FINGERPRINT_OK) {
      display.clearBuffer(); 
      hienthi_oled("Xoa thanh cong!",30,10);
       display.sendBuffer(); 
  } else {
      display.clearBuffer();
      hienthi_oled("Khong the xoa ID nay!",20,10);
      display.sendBuffer();
  }
}

void ketnoilaivantay(){
    // KET NOI VAN TAY
     van_tay_Serial.begin(57600, SERIAL_8N1, 16, 17); // Kết nối      UART2 với AS608
     display.clearBuffer();
     hienthi_oled("-Dang kiem tra ket",0,12);
      hienthi_oled("noi van tay",0,24);
     display.sendBuffer(); 
     delay(2000);  
    //Serial.println("Đang kiểm tra kết nối với cảm biến vân tay...");
    if (van_tay.verifyPassword()) {
      display.clearBuffer();
      hienthi_oled("Ket noi thanh cong",0,20);
      display.sendBuffer(); 
    
      
    delay(2000);  
      //Serial.println(" Cảm biến vân tay kết nối thành công!");
    } else {    
        display.clearBuffer();
        hienthi_oled("Ket noi",0,12);
        hienthi_oled(" khong thanh cong ",0,24);
        display.sendBuffer();
    
        delay(2000);
     
    }
}
void mo_khoa(){    
     display.clearBuffer();
     hienthi_oled("Khoa dang mo",0,20);
     display.sendBuffer();
     loakeu();
     digitalWrite(khoa,1);
     delay(4000);
     loakeu();

     
     
    
     digitalWrite(khoa,0);
}
void hienthi_oled(char *m , int x , int y){      
      display.setFont(u8g2_font_ncenB08_tr);  // Chọn font
       display.drawStr(x, y, m);
}
void themvantay(int id){
    if (van_tay.loadModel(id) == FINGERPRINT_OK)
    { 
       display.clearBuffer();
       hienthi_oled("ID nay da duoc dang ky",0,20);
       display.sendBuffer();
       delay(2000);
       return;
     }  
    dang_ky_van_tay(id);

  // char idst[3];
  // sprintf(idst, "%d", id);

 }
