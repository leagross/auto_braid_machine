//  FirebaseManager.ino
//
//  NOTE: Firebase.Firestore.* / Firebase.begin() / Firebase.signUp() calls in
//  this file are synchronous network I/O from the Firebase_ESP_Client library
//  — they block for the duration of the HTTPS request (typically well under a
//  second, but not instant). The rest of the app (second_esp32.ino's state
//  machine, motor ticks, touch screen) is fully non-blocking, but these calls
//  are the one remaining exception: making them async too would require
//  switching to the library's lower-level async request API, which is a
//  bigger change than this refactor covers. In practice this only matters at
//  the couple of points where a session enters ST_VALIDATING_CODE or saves an
//  order — the WebServer.ino HTTP endpoints for the app run on Core 0, a
//  separate task, so they are unaffected either way.
#include "Config.h"
#include <WiFi.h>                    // network connection
#include <Firebase_ESP_Client.h>     // Firebase library
#include "addons/TokenHelper.h"      // login token helper

FirebaseData   fbdo;                 // data object (not static — also needed in AuthManager.ino)
static FirebaseAuth   fbAuth;        // login details
static FirebaseConfig fbConfig;      // settings
static bool fbReady = false;         // true only if Firebase came up successfully

// Helper: extract a text field (stringValue) from a Firestore document
static String fsStr(FirebaseJson& doc, const String& fieldName) {
  FirebaseJsonData d;
  doc.get(d, "fields/" + fieldName + "/stringValue");
  return d.success ? d.to<String>() : "";
}

// Helper: extract a boolean field (booleanValue) from a Firestore document
static bool fsBool(FirebaseJson& doc, const String& fieldName) {
  FirebaseJsonData d;
  doc.get(d, "fields/" + fieldName + "/booleanValue");
  return d.success && d.to<bool>();
}

// Init — called from the main setup()
void firebaseSetup() {
  Serial.println("[FB] Action: connecting to WiFi...");
  if (String(WIFI_SSID) == "YOUR_WIFI") {      // not configured
    Serial.println("[FB] Result: not configured -> DEMO mode");
    fbReady = false; return;
  }
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);         // connect to WiFi
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 10000) delay(300);  // up to 10s
  if (WiFi.status() != WL_CONNECTED) {          // failed
    Serial.println("[FB] Result: WiFi FAILED -> DEMO mode");
    fbReady = false; return;
  }
  Serial.print("[FB] Result: WiFi OK, IP="); Serial.println(WiFi.localIP());

  fbConfig.api_key = FB_API_KEY;                // API key (enough for Firestore, no database_url needed)
  fbConfig.token_status_callback = tokenStatusCallback;

  fbAuth.user.email    = FB_DEVICE_EMAIL;       // device account
  fbAuth.user.password  = FB_DEVICE_PASS;

  // Important: don't poll Firebase.ready() in a tight loop! Each check can
  // make the library send a new auth request to Google. Polling too often
  // (every 200ms) once caused dozens of requests within seconds and a
  // temporary block from Google (TOO_MANY_ATTEMPTS_TRY_LATER). So: a single
  // begin() call, a fixed wait, then one single ready() check.
  Serial.println("[FB] Action: signing in device account...");
  Firebase.begin(&fbConfig, &fbAuth);           // attempt sign-in (once only)
  Firebase.reconnectWiFi(true);                 // auto reconnect

  delay(4000);                                  // wait without polling ready() in a loop

  if (!Firebase.ready()) {                      // single check
    // maybe the account doesn't exist yet — try creating it (one-time signup)
    Serial.println("[FB] Result: sign-in failed, trying to create device account...");
    if (Firebase.signUp(&fbConfig, &fbAuth, FB_DEVICE_EMAIL, FB_DEVICE_PASS)) {
      Serial.println("[FB] Result: device account created");
    } else {
      Serial.print("[FB] Result: signUp FAILED: ");
      Serial.println(fbConfig.signer.signupError.message.c_str());
    }
    delay(4000);                                // one more wait, no loop
  }

  fbReady = Firebase.ready();                   // final single check
  Serial.println(fbReady ? "[FB] Result: Firestore ready" : "[FB] Result: NOT ready -> DEMO mode");
}

// Validates a code. Returns true if valid and fills name+uid.
bool fbValidateCode(const String& code, String& name, String& uid) {
  Serial.println("[FB] Action: validating code " + code + " ...");
  if (!fbReady) {                    // demo mode
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

// Releases a code after successful use: deletes the codes/{code} document
// entirely (not just marking used=true) — so the 4-digit number becomes free
// for reuse (there are only 9000 possible combinations; without releasing
// them they would eventually run out).
void fbReleaseCode(const String& code) {
  if (!fbReady) return;
  Firebase.Firestore.deleteDocument(&fbdo, FIREBASE_PROJECT_ID, "", ("codes/" + code).c_str());
  Serial.println("[FB] Action: code " + code + " released (deleted, free for reuse)");
}

// Helper: extract a numeric field (integerValue) from a Firestore document
static long fsInt(FirebaseJson& doc, const String& fieldName) {
  FirebaseJsonData d;
  doc.get(d, "fields/" + fieldName + "/integerValue");
  return d.success ? d.to<int>() : 0;
}

// Helper: turns a single Firestore document into simple JSON (the format the
// app expects) and appends it to the output
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

// Fetches orders from Firestore and returns JSON ready to send to the app:
// "[{...},{...}]". Non-empty uidFilter -> only that uid's orders (customer);
// empty -> all orders (admin).
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
    if (uidFilter.length() && fsStr(doc, "uid") != uidFilter) continue;   // filter by user
    appendOrderJson(doc, docName, out);
  }
  return "[" + out + "]";
}

// Deletes a single order by document id (orders/{orderId})
bool fbDeleteOrder(const String& orderId) {
  if (!fbReady) return true;
  bool ok = Firebase.Firestore.deleteDocument(&fbdo, FIREBASE_PROJECT_ID, "", ("orders/" + orderId).c_str());
  if (!ok) Serial.println("[FB] Result: deleteOrder FAILED: " + fbdo.errorReason());
  return ok;
}

// Saves a new order under orders/{orderId}.
// status: "completed" = braid finished successfully | "emergency" = stopped in an emergency.
void fbSaveOrder(const String& uid, const String& name,
                 const String& extensions, const String& hairColor,
                 const String& status) {
  Serial.println("[FB] Action: saving order (" + status + ")... name=" + name +
                  " ext=" + extensions + " hair=" + hairColor);
  if (!fbReady) {                    // demo mode — just log it
    Serial.printf("[FB][DEMO] Result: order NOT saved (no Firebase): %s | %s | %s | %s\n",
                  name.c_str(), extensions.c_str(), hairColor.c_str(), status.c_str());
    return;
  }
  String orderId = uid + "_" + String(millis());  // unique id (uid + time)

  FirebaseJson json;                 // builds the Firestore document
  json.set("fields/uid/stringValue",        uid);
  json.set("fields/name/stringValue",       name);
  json.set("fields/extensions/stringValue", extensions);
  json.set("fields/hairColor/stringValue",  hairColor);
  json.set("fields/status/stringValue",     status);          // "completed" / "emergency"
  json.set("fields/createdAt/integerValue", String(millis()));  // time relative to board boot

  if (Firebase.Firestore.createDocument(&fbdo, FIREBASE_PROJECT_ID, "",
        ("orders/" + orderId).c_str(), json.raw())) {
    Serial.println("[FB] Result: order saved, id=" + orderId);
  } else {
    Serial.println("[FB] Result: order save FAILED: " + fbdo.errorReason());
  }
}
