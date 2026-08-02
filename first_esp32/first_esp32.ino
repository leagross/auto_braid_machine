// ============================================================================
//    Config.h             
//    first_esp32.ino          — לוגיקה
//    buttom_switch.ino        — כפתור התחלה/חירום
//    Ultrasonic_esp32.ino     — חיישן אולטרסוני
//    TCS34725_Color_sensor.ino — חיישן צבע (I2C)
//    Dispenser.ino            — מנוע תוספות (הועבר לכאן מהלוח השני)
// ============================================================================
#include "Config.h"                 

bool     armed          = false;    // האם אנחנו במצב קליעה (מנטרים חירום)
bool     lastBtnPressed = false;    // מצב הכפתור בסבב הקודם (לזיהוי לחיצה חדשה)
uint32_t lastBtnChange  = 0;        // חותמת זמן הלחיצה האחרונה (לדיבאונס)
String   rxLine         = "";       // בופר לשורה הנכנסת מה-UART

void setup() {
  Serial.begin(115200);             
  Serial2.begin(UART_BAUD, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);

  buttonSetup();                  
  ultrasonicSetup();                
  colorSensorSetup();
  dispenserSetup();         

  Serial.println("\n[FIRST] Sensors board ready");
}

void loop() {
  handleUart();                     // מטפל בפקודות שמגיעות מהלוח השני
  handleButton();                   // מנטר את כפתור החירום
}

//  handleUart —קליטת פקודה מהלוח השני
void handleUart() {
  while (Serial2.available()) {      
    char c = Serial2.read();         
    if (c == '\n' || c == '\r') {    
      if (rxLine.length() > 0) {     
        processCommand(rxLine);      // מפענח ומבצע
        rxLine = "";                 
      }
    } else {                         
      rxLine += c;                   
      if (rxLine.length() > 40) rxLine = "";  // הגנה מפני זבל
    }
  }
}

//  processCommand — מחליט מה לעשות לפי הפקודה

void processCommand(const String& cmd) {
  Serial.print("[FIRST] cmd: "); Serial.println(cmd);

  if (cmd == "DIST?") {                        
    float d = readDistanceCm();                
    bool ok = (d >= DIST_MIN_CM && d <= DIST_MAX_CM);  
    sendLine("DIST:" + String(d, 1));          
    sendLine("DISTOK:" + String(ok ? 1 : 0));  
  }
  else if (cmd == "COLOR?") {                  
    String name = readHairColor();             
    sendLine("COLOR:" + name);                
  }
  else if (cmd == "ARMED") {                   // התחילה קליעה
    armed = true; lastBtnPressed = false;      
    Serial.println("[FIRST] ARMED");
  }
  else if (cmd == "DISARM") {                  // הקליעה הסתיימה
    armed = false;                             // מכבה ניטור חירום
    Serial.println("[FIRST] DISARM");
  }
  else if (cmd == "PING") {                    // בדיקת קשר
    sendLine("PONG");
  }
  else if (cmd.startsWith("DISP:")) {          // בקשת סיבוב קרוסלת התוספות
    int idx = cmd.substring(5).toInt();         
    dispenserGoTo(idx);                       
    sendLine("DISPDONE");                      // מדווח שסיים
  }
}

//  handleButton — מזהה לחיצה חדשה ומדווח לפי המצב
void handleButton() {
  bool pressed = buttonPressed();

  // לחוץ 
  if (pressed && !lastBtnPressed && (millis() - lastBtnChange > BTN_DEBOUNCE_MS)) {
    lastBtnChange = millis();       
    if (armed) {                     // אם באמצע קליעה
      sendLine("EMG");               // שולח עצירת חירום
      Serial.println("[FIRST] >>> EMERGENCY");
    } else {                         
      sendLine("OK");
      Serial.println("[FIRST] >>> OK/clear");
    }
  }
  lastBtnPressed = pressed;          
}

//  sendLine טקסט ללוח השני
void sendLine(const String& s) {
  Serial2.print(s);               
  Serial2.print('\n');               
}
