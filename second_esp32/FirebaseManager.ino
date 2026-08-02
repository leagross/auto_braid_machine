//  FirebaseManager.ino
#include "Config.h"
#include <WiFi.h>                    // חיבור לרשת
#include <Firebase_ESP_Client.h>     // ספריית Firebase
#include "addons/TokenHelper.h"      // עזר לטוקן ההתחברות

FirebaseData   fbdo;                 // אובייקט נתונים (לא static — נדרש גם ב-AuthManager.ino)
static FirebaseAuth   fbAuth;        // פרטי התחברות
static FirebaseConfig fbConfig;      // הגדרות
static bool fbReady = false;         // true רק אם Firebase עלה בהצלחה

// עוזר: חילוץ שדה טקסט (stringValue) ממסמך Firestore
static String fsStr(FirebaseJson& doc, const String& fieldName) {
  FirebaseJsonData d;
  doc.get(d, "fields/" + fieldName + "/stringValue");
  return d.success ? d.to<String>() : "";
}

// עוזר: חילוץ שדה בוליאני (booleanValue) ממסמך Firestore
static bool fsBool(FirebaseJson& doc, const String& fieldName) {
  FirebaseJsonData d;
  doc.get(d, "fields/" + fieldName + "/booleanValue");
  return d.success && d.to<bool>();
}

// אתחול — נקרא מ-setup() הראשי
void firebaseSetup() {
  Serial.println("[FB] Action: connecting to WiFi...");
  if (String(WIFI_SSID) == "YOUR_WIFI") {      // לא מולאו פרטים
    Serial.println("[FB] Result: not configured -> DEMO mode");
    fbReady = false; return;
  }
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);         // מתחבר ל-WiFi
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 10000) delay(300);  // עד 10ש'
  if (WiFi.status() != WL_CONNECTED) {          // נכשל
    Serial.println("[FB] Result: WiFi FAILED -> DEMO mode");
    fbReady = false; return;
  }
  Serial.print("[FB] Result: WiFi OK, IP="); Serial.println(WiFi.localIP());

  fbConfig.api_key = FB_API_KEY;                // מפתח API (מספיק ל-Firestore, בלי database_url)
  fbConfig.token_status_callback = tokenStatusCallback;

  fbAuth.user.email    = FB_DEVICE_EMAIL;       // חשבון המכשיר
  fbAuth.user.password  = FB_DEVICE_PASS;

  // ⚠️ חשוב: לא לבדוק Firebase.ready() בלולאה צפופה! כל בדיקה כזו יכולה לגרום
  // לספרייה לשלוח בקשת אימות חדשה לגוגל. בדיקה תכופה מדי (כל 200 מ"ש) גרמה
  // לעשרות בקשות תוך שניות וחסימה זמנית ע"י גוגל (TOO_MANY_ATTEMPTS_TRY_LATER).
  // לכן: קריאה אחת בלבד ל-begin(), המתנה קבועה, ובדיקה בודדת של ready().
  Serial.println("[FB] Action: signing in device account...");
  Firebase.begin(&fbConfig, &fbAuth);           // מנסה כניסה (פעם אחת בלבד)
  Firebase.reconnectWiFi(true);                 // חיבור מחדש אוטומטי

  delay(4000);                                  // ממתין בלי לבדוק ready() בלולאה

  if (!Firebase.ready()) {                      // בדיקה בודדת
    // אולי החשבון עוד לא קיים — מנסה ליצור אותו (הרשמה חד-פעמית, פעם אחת בלבד)
    Serial.println("[FB] Result: sign-in failed, trying to create device account...");
    if (Firebase.signUp(&fbConfig, &fbAuth, FB_DEVICE_EMAIL, FB_DEVICE_PASS)) {
      Serial.println("[FB] Result: device account created");
    } else {
      Serial.print("[FB] Result: signUp FAILED: ");
      Serial.println(fbConfig.signer.signupError.message.c_str());
    }
    delay(4000);                                // המתנה נוספת, בלי לולאה
  }

  fbReady = Firebase.ready();                   // בדיקה בודדת סופית
  Serial.println(fbReady ? "[FB] Result: Firestore ready" : "[FB] Result: NOT ready -> DEMO mode");
}

// מאמת קוד. מחזיר true אם תקין וממלא name+uid.
bool fbValidateCode(const String& code, String& name, String& uid) {
  Serial.println("[FB] Action: validating code " + code + " ...");
  if (!fbReady) {                    // מצב דמו
    name = "Guest"; uid = "demo";
    bool ok = (code.length() == CODE_LENGTH);
    Serial.println(String("[FB][DEMO] Result: ") + (ok ? "ACCEPTED" : "REJECTED"));
    return ok;
  }

  if (!Firebase.Firestore.getDocument(&fbdo, FIREBASE_PROJECT_ID, "",
        ("codes/" + code).c_str(), "")) {
    Serial.println("[FB] Result: code NOT FOUND (" + fbdo.errorReason() + ")");
    return false;
  }

  FirebaseJson doc;
  doc.setJsonData(fbdo.payload());
  name       = fsStr(doc, "name");
  uid        = fsStr(doc, "uid");
  bool used  = fsBool(doc, "used");

  if (used) {
    Serial.println("[FB] Result: code ALREADY USED, name=" + name);
    return false;
  }
  Serial.println("[FB] Result: code OK, name=" + name + " uid=" + uid);
  return true;
}

// משחרר קוד אחרי שימוש מוצלח: מוחק את מסמך codes/{code} לגמרי (לא רק מסמן
// used=true) — כך שהמספר בן 4 הספרות חוזר להיות פנוי לשימוש חוזר להגרלה
// הבאה (יש רק 9000 קומבינציות אפשריות, בלי שחרור הן היו "נגמרות" עם הזמן).
void fbReleaseCode(const String& code) {
  if (!fbReady) return;
  Firebase.Firestore.deleteDocument(&fbdo, FIREBASE_PROJECT_ID, "", ("codes/" + code).c_str());
  Serial.println("[FB] Action: code " + code + " released (deleted, free for reuse)");
}

// עוזר: חילוץ שדה מספרי (integerValue) ממסמך Firestore
static long fsInt(FirebaseJson& doc, const String& fieldName) {
  FirebaseJsonData d;
  doc.get(d, "fields/" + fieldName + "/integerValue");
  return d.success ? d.to<int>() : 0;
}

// עוזר: הופך מסמך Firestore בודד ל-JSON פשוט (בפורמט שהאפליקציה מצפה לו), ומצרף לפלט
static void appendOrderJson(FirebaseJson& doc, const String& docName, String& out) {
  String id = docName.substring(docName.lastIndexOf('/') + 1);
  if (out.length()) out += ",";
  out += "{\"id\":\"" + id + "\"";
  out += ",\"uid\":\"" + fsStr(doc, "uid") + "\"";
  out += ",\"name\":\"" + fsStr(doc, "name") + "\"";
  out += ",\"extensions\":\"" + fsStr(doc, "extensions") + "\"";
  out += ",\"hairColor\":\"" + fsStr(doc, "hairColor") + "\"";
  out += ",\"status\":\"" + fsStr(doc, "status") + "\"";
  out += ",\"createdAt\":" + String(fsInt(doc, "createdAt"));
  out += "}";
}

// שולף הזמנות מ-Firestore ומחזיר JSON מוכן לשליחה לאפליקציה: "[{...},{...}]".
// uidFilter לא ריק -> רק ההזמנות של אותו uid (לקוח); ריק -> כל ההזמנות (מנהל).
String fbListOrders(const String& uidFilter) {
  if (!fbReady) return "[]";
  if (!Firebase.Firestore.listDocuments(&fbdo, FIREBASE_PROJECT_ID, "", "orders", 300, "", "", "", false)) {
    Serial.println("[FB] Result: listOrders FAILED: " + fbdo.errorReason());
    return "[]";
  }
  FirebaseJson result;
  result.setJsonData(fbdo.payload());
  FirebaseJsonData listField;
  result.get(listField, "documents");
  if (!listField.success) return "[]";

  FirebaseJsonArray arr;
  listField.get<FirebaseJsonArray>(arr);

  String out = "";
  for (size_t i = 0; i < arr.size(); i++) {
    FirebaseJsonData item;
    arr.get(item, i);
    FirebaseJson doc;
    doc.setJsonData(item.to<String>());
    FirebaseJsonData nameField;
    doc.get(nameField, "name");
    String docName = nameField.to<String>();
    if (uidFilter.length() && fsStr(doc, "uid") != uidFilter) continue;   // סינון לפי משתמש
    appendOrderJson(doc, docName, out);
  }
  return "[" + out + "]";
}

// מוחק הזמנה בודדת לפי מזהה המסמך (orders/{orderId})
bool fbDeleteOrder(const String& orderId) {
  if (!fbReady) return true;
  bool ok = Firebase.Firestore.deleteDocument(&fbdo, FIREBASE_PROJECT_ID, "", ("orders/" + orderId).c_str());
  if (!ok) Serial.println("[FB] Result: deleteOrder FAILED: " + fbdo.errorReason());
  return ok;
}

// שומר הזמנה חדשה תחת orders/{orderId}.
// status: "completed" = קליעה הסתיימה בהצלחה | "emergency" = נעצרה בחירום.
void fbSaveOrder(const String& uid, const String& name,
                 const String& extensions, const String& hairColor,
                 const String& status) {
  Serial.println("[FB] Action: saving order (" + status + ")... name=" + name +
                  " ext=" + extensions + " hair=" + hairColor);
  if (!fbReady) {                    // מצב דמו — רק מדפיס
    Serial.printf("[FB][DEMO] Result: order NOT saved (no Firebase): %s | %s | %s | %s\n",
                  name.c_str(), extensions.c_str(), hairColor.c_str(), status.c_str());
    return;
  }
  String orderId = uid + "_" + String(millis());  // מזהה ייחודי (uid + זמן)

  FirebaseJson json;                 // בונה את מסמך ה-Firestore
  json.set("fields/uid/stringValue",        uid);
  json.set("fields/name/stringValue",       name);
  json.set("fields/extensions/stringValue", extensions);
  json.set("fields/hairColor/stringValue",  hairColor);
  json.set("fields/status/stringValue",     status);          // "completed" / "emergency"
  json.set("fields/createdAt/integerValue", String(millis()));  // זמן יחסי מהדלקת הלוח

  if (Firebase.Firestore.createDocument(&fbdo, FIREBASE_PROJECT_ID, "",
        ("orders/" + orderId).c_str(), json.raw())) {
    Serial.println("[FB] Result: order saved, id=" + orderId);
  } else {
    Serial.println("[FB] Result: order save FAILED: " + fbdo.errorReason());
  }
}
