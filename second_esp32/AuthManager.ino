// ============================================================================
//  AuthManager.ino  —  לוגיקת משתמשים (הרשמה/כניסה/קוד זמני), כל התקשורת
//  הרשמה/כניסה: מול Firebase Auth REST API (HTTPClient), בנפרד מחשבון ה"מכשיר"
//  כתיבה/קריאה של users/codes: דרך Firestore עם הרשאות חשבון המכשיר (fbdo הקיים).
// ============================================================================
#include "Config.h"
#include <HTTPClient.h>

static FirebaseData authFbdo;

static String fsStrGet(FirebaseJson& doc, const String& fieldName) {
  FirebaseJsonData d;
  doc.get(d, "fields/" + fieldName + "/stringValue");
  return d.success ? d.to<String>() : "";
}

// קריאה ל-Firebase Auth REST (signUp/signIn מחזירים idToken+localId ב-JSON)
static bool authRest(const String& endpoint, const String& email, const String& pass, String& uidOut, String& errOut) {
  HTTPClient http;
  String url = "https://identitytoolkit.googleapis.com/v1/accounts:" + endpoint + "?key=" + String(FB_API_KEY);
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  String body = "{\"email\":\"" + email + "\",\"password\":\"" + pass + "\",\"returnSecureToken\":true}";
  int code = http.POST(body);
  String resp = http.getString();
  http.end();

  FirebaseJson json; json.setJsonData(resp);
  FirebaseJsonData d;
  if (code == 200) {
    json.get(d, "localId"); uidOut = d.success ? d.to<String>() : "";
    return uidOut.length() > 0;
  }
  json.get(d, "error/message");
  errOut = d.success ? d.to<String>() : "AUTH_ERROR";
  return false;
}

// ---- הרשמה: יוצר חשבון Auth + מסמך users/{uid} ----
bool authRegister(const String& email, const String& pass, const String& name, String& uidOut, String& errOut) {
  if (!authRest("signUp", email, pass, uidOut, errOut)) return false;

  FirebaseJson json;
  json.set("fields/name/stringValue", name);
  json.set("fields/email/stringValue", email);
  json.set("fields/role/stringValue", "user");
  Firebase.Firestore.createDocument(&authFbdo, FIREBASE_PROJECT_ID, "", ("users/" + uidOut).c_str(), json.raw());
  return true;
}

// ---- כניסה: מאמת מול Auth ואז שולף name+role מ-Firestore ----
bool authLogin(const String& email, const String& pass, String& uidOut, String& nameOut, String& roleOut, String& errOut) {
  if (!authRest("signInWithPassword", email, pass, uidOut, errOut)) return false;

  if (!Firebase.Firestore.getDocument(&authFbdo, FIREBASE_PROJECT_ID, "", ("users/" + uidOut).c_str(), "")) {
    nameOut = email; roleOut = "user"; return true;   // אין מסמך
  }
  FirebaseJson doc; doc.setJsonData(authFbdo.payload());
  nameOut = fsStrGet(doc, "name"); if (nameOut.length() == 0) nameOut = email;
  roleOut = fsStrGet(doc, "role"); if (roleOut.length() == 0) roleOut = "user";
  return true;
}

// ---- יצירת קוד זמני: מגריל, מוודא שאינו תפוס ב-Firestore, שומר ----
String authGenerateCode(const String& uid, const String& name) {
  String code;
  for (int tries = 0; tries < 10; tries++) {
    code = String(1000 + random(9000));                 // 4 ספרות
    if (!Firebase.Firestore.getDocument(&authFbdo, FIREBASE_PROJECT_ID, "", ("codes/" + code).c_str(), ""))
      break;                                             // אין מסמך כזה => פנוי
  }
  FirebaseJson json;
  json.set("fields/uid/stringValue", uid);
  json.set("fields/name/stringValue", name);
  json.set("fields/used/booleanValue", false);
  Firebase.Firestore.createDocument(&authFbdo, FIREBASE_PROJECT_ID, "", ("codes/" + code).c_str(), json.raw());
  return code;
}
