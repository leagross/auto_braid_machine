// ============================================================================
//  second_esp32.ino  (Master)  —  הקובץ הראשי
//  ----------------------------------------------------------------------------
//    Config.h            — פינים וקבועים
//    second_esp32.ino    — setup/loop + כל זרימת הלוגיקה (הקובץ הזה)
//    DisplayManager.ino  —  מסך המגע (מקלדת, בחירת תוספות, הודעות) ← "המסכים"
//    Motors.ino          — מנועי הצעד של הלוח הזה (מסילה / קליעה)
//    UartLink.ino        — תקשורת עם הלוח הראשון (חיישנים + חירום + קרוסלת תוספות)
//    FirebaseManager.ino — WiFi + Firebase (אימות קוד + שמירת הזמנה) ← "פייר בייס"
// ============================================================================
#include "Config.h"

//  setup — אתחול כל המערכות
void setup() {
  Serial.begin(115200);              
  Serial.println("\n[SECOND] Master starting");

  displaySetup();                    
  motorsSetup();                     
  uartSetup();                       

  uiMessage("Starting...", "Please wait"); 
  firebaseSetup();                   
  webServerSetup();                  

  requestDispenserGoTo(0, 4000);      
  //. לאיפוס : railUpHome();
}

void loop() {
  runSession();        
}

//  runSession — כל זרימת הלוגיקה של משתמש אחד
void runSession() {
  String name, uid, code;         

  Serial.println("\n========== [SECOND] NEW SESSION ==========");

  // ---- 1+2: הקלדת קוד ואימות מול Firebase ----
  Serial.println("[SECOND] Stage 1: waiting for code entry...");
  while (true) {
    code = uiGetCode();              
    Serial.println("[SECOND] Action: code entered = " + code);
    uiMessage("Checking...", "");
    if (fbValidateCode(code, name, uid)) break;  // Firebase מאמת ומחזיר שם
    Serial.println("[SECOND] Result: code REJECTED, asking again");
    uiMessage("Invalid code", "Try again");
    delay(1500);
  }
  Serial.println("[SECOND] Result: logged in as " + name + " (uid=" + uid + ")");

  uiMessage("Connected!", "Code verified in database");
  delay(1200);

  // ---- 3: המערכת מוכנה + בחירת תוספות ----
  Serial.println("[SECOND] Stage 2: waiting for extension selection...");
  int tokens[MAX_EXTENSIONS];        // הטוקנים של התוספות שנבחרו
  int count = uiSelectExtensions(name, tokens);  // מסך הבחירה -> כמה נבחרו
  Serial.printf("[SECOND] Result: %d extension(s) selected\n", count);

  // ---- 4: הכנס שיער ----
  Serial.println("[SECOND] Stage 3: waiting for hair insertion + START...");
  uiInsertHair();                    // כפתור START

  // ---- 5: בדיקת מרחק (אולטרסוני בלוח הראשון) ----
  Serial.println("[SECOND] Stage 4: distance check...");
  uiMessage("Checking", "distance...");
  while (!requestDistanceOk(1500)) { // חוזר עד שהמרחק תקין
    uiMessage("Bad distance", "Please adjust");
    delay(1500);
  }

  // ---- 5b: פתירת "כצבע שערי" בעזרת חיישן הצבע ----
  String hairColor = "-";
  for (int i = 0; i < count; i++) {
    if (tokens[i] == EXT_MYHAIR) {   // אם נבחר "כצבע שערי"
      Serial.println("[SECOND] Stage 5: MyHair selected -> scanning color...");
      uiMessage("Scanning", "hair color...");
      hairColor = requestHairColor(4000);      
      tokens[i] = colorNameToIndex(hairColor); // הופך לאינדקס מקום
      Serial.println("[SECOND] Result: MyHair resolved to -> " + hairColor);

      uiMessage("Detected:", hairColor.c_str());
      delay(1800);
    }
  }

  // ---- 6: הגשת התוספות (רבע סיבוב לכל צבע ) ----
  Serial.println("[SECOND] Stage 6: dispensing extensions...");
  for (int i = 0; i < count; i++) {
    if (tokens[i] == EXT_NONE) continue;       
    Serial.println("[SECOND] Action: dispensing " + String(extName(tokens[i])));
    requestDispenserGoTo(tokens[i], 4000);      // מבקש מהראשון לסובב למקום הצבע
    uiTakeExtension(extName(tokens[i]));        // מבקש מהמשתמש לקחת
  }
  requestDispenserGoTo(0, 4000);      // מחזיר קרוסלה למקום התחלתי

  // בונה את רשימת התוספות לשמירה כבר עכשיו — נדרש גם אם קורה חירום בהמשך
  String extCsv = "";
  for (int i = 0; i < count; i++) {
    if (tokens[i] == EXT_NONE) continue;
    if (extCsv.length()) extCsv += ",";
    extCsv += extName(tokens[i]);
  }
  if (extCsv.length() == 0) extCsv = "None";

  // ---- 6b: מחכה ללחיצת אישור מפורשת לפני שהמנועים זזים ----
  Serial.println("[SECOND] Stage 6b: waiting for braid START confirmation...");
  uiReadyToBraid();                 

  // ---- 7: קליעה — מסילה יורדת + מנוע קליעה מסתובב ----
  Serial.println("[SECOND] Stage 7: braiding...");
  uiMessage("Braiding...", "Press btn = stop");
  uartArm();                         // מבקש מהראשון להתחיל לנטר חירום (מאפס דגל)
  bool finished = braidWhileLowering();        // רץ עד למטה (סוף המסילה) או עד חירום
  uartDisarm();                      // מבקש מהראשון להפסיק לנטר

  // ---- 8: טיפול בחירום ----
  if (!finished) {                  
    Serial.println("[SECOND] Stage 8: EMERGENCY handling...");
    motorsStopAll();                 
    uiEmergency();                  
    Serial.println("[SECOND] Action: emergency cleared, resetting...");
    uiMessage("Resetting...", "");
    railUpHome();                    // איפוס: המסילה עולה

    // אותה בלשונית נפרדת. הקוד *לא* מסומן כמנוצל, כדי שהלקוח יוכל לנסות שוב.
    fbSaveOrder(uid, name, extCsv, hairColor, "emergency");

    Serial.println("========== [SECOND] SESSION ABORTED (emergency) ==========\n");
    return;                        
  }

  // ---- 9: סיום תקין ----
  Serial.println("[SECOND] Stage 9: braid complete (motor reached end of rail)");
  uiDone();                          
  Serial.println("[SECOND] Stage 9b: finishing + saving order...");
  uiMessage("Finishing...", "");
  railUpHome();                      // המסילה עולה למעלה, חוזרת למצב התחלתי

  fbSaveOrder(uid, name, extCsv, hairColor, "completed"); 
  fbReleaseCode(code);                // מוחק את הקוד -> חוזר להיות פנוי לשימוש חוזר

  uiMessage("Saved!", name.c_str());          
  Serial.println("========== [SECOND] SESSION DONE ==========\n");
  delay(2500);
}

// ===========================================================================
//  colorNameToIndex — המרת שם צבע (מהחיישן) לאינדקס מקום בקרוסלה
// ===========================================================================
int colorNameToIndex(const String& name) {
  if (name == "Blonde") return EXT_BLONDE;
  if (name == "Green")  return EXT_GREEN;
  if (name == "Red")    return EXT_RED;
  if (name == "Black")  return EXT_BLACK;
  return EXT_NONE;
}
