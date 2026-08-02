// ============================================================================
//  AuthManager.ino — user logic (register/login/temporary code). Registration
//  and login go through the Firebase Auth REST API (HTTPClient), separate
//  from the "device" account; reading/writing users/codes goes through
//  Firestore using the device account's permissions (the existing fbdo).
// ============================================================================
#include "Config.h"
#include <HTTPClient.h>

static FirebaseData authFbdo;

static String fsStrGet(FirebaseJson& doc, const String& fieldName) {
  FirebaseJsonData d;
  doc.get(d, "fields/" + fieldName + "/stringValue");
  return d.success ? d.to<String>() : "";
}

// Calls the Firebase Auth REST API (signUp/signIn return idToken+localId as JSON)
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

// ---- Register: creates an Auth account + a users/{uid} document ----
bool authRegister(const String& email, const String& pass, const String& name, String& uidOut, String& errOut) {
  if (!authRest("signUp", email, pass, uidOut, errOut)) return false;

  FirebaseJson json;
  json.set("fields/name/stringValue", name);
  json.set("fields/email/stringValue", email);
  json.set("fields/role/stringValue", "user");
  Firebase.Firestore.createDocument(&authFbdo, FIREBASE_PROJECT_ID, "", ("users/" + uidOut).c_str(), json.raw());
  return true;
}

// ---- Login: authenticates against Auth, then fetches name+role from Firestore ----
bool authLogin(const String& email, const String& pass, String& uidOut, String& nameOut, String& roleOut, String& errOut) {
  if (!authRest("signInWithPassword", email, pass, uidOut, errOut)) return false;

  if (!Firebase.Firestore.getDocument(&authFbdo, FIREBASE_PROJECT_ID, "", ("users/" + uidOut).c_str(), "")) {
    nameOut = email; roleOut = "user"; return true;   // no document
  }
  FirebaseJson doc; doc.setJsonData(authFbdo.payload());
  nameOut = fsStrGet(doc, "name"); if (nameOut.length() == 0) nameOut = email;
  roleOut = fsStrGet(doc, "role"); if (roleOut.length() == 0) roleOut = "user";
  return true;
}

// ---- Generates a temporary code: rolls one, checks it's free in Firestore, saves it ----
String authGenerateCode(const String& uid, const String& name) {
  String code;
  for (int tries = 0; tries < 10; tries++) {
    code = String(1000 + random(9000));                 // 4 digits
    if (!Firebase.Firestore.getDocument(&authFbdo, FIREBASE_PROJECT_ID, "", ("codes/" + code).c_str(), ""))
      break;                                             // no such document => free
  }
  FirebaseJson json;
  json.set("fields/uid/stringValue", uid);
  json.set("fields/name/stringValue", name);
  json.set("fields/used/booleanValue", false);
  Firebase.Firestore.createDocument(&authFbdo, FIREBASE_PROJECT_ID, "", ("codes/" + code).c_str(), json.raw());
  return code;
}
