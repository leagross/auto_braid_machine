// ============================================================================
//  WebServer.ino — HTTP server for the React app: /register, /login,
//  /generate-code, /my-orders, /all-orders, /delete-order. Everything goes
//  through Firestore via AuthManager.ino/FirebaseManager.ino — the app never
//  talks to Firebase directly.
// ============================================================================
#include "Config.h"
#include <WebServer.h>

static WebServer authServer(80);

static void sendJson(int code, const String& body) {
  authServer.sendHeader("Access-Control-Allow-Origin", "*");
  authServer.sendHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
  authServer.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  authServer.send(code, "application/json", body);
}

static void handleCors() { sendJson(200, "{}"); }

static bool bodyField(FirebaseJsonData& d, const char* field) {
  FirebaseJson body;
  body.setJsonData(authServer.arg("plain"));
  body.get(d, field);
  return d.success;
}

static void handleRegister() {
  FirebaseJsonData e, p, n;
  if (!bodyField(e, "email") || !bodyField(p, "password") || !bodyField(n, "name")) {
    sendJson(400, "{\"message\":\"missing fields\"}"); return;
  }
  String uid, err;
  if (!authRegister(e.to<String>(), p.to<String>(), n.to<String>(), uid, err)) {
    sendJson(400, "{\"message\":\"" + err + "\"}"); return;
  }
  sendJson(201, "{\"uid\":\"" + uid + "\",\"name\":\"" + n.to<String>() + "\",\"role\":\"user\"}");
}

static void handleLogin() {
  FirebaseJsonData e, p;
  if (!bodyField(e, "email") || !bodyField(p, "password")) {
    sendJson(400, "{\"message\":\"missing fields\"}"); return;
  }
  String uid, name, role, err;
  if (!authLogin(e.to<String>(), p.to<String>(), uid, name, role, err)) {
    sendJson(401, "{\"message\":\"" + err + "\"}"); return;
  }
  sendJson(200, "{\"uid\":\"" + uid + "\",\"name\":\"" + name + "\",\"role\":\"" + role + "\"}");
}

static void handleGenerateCode() {
  FirebaseJsonData u, n;
  if (!bodyField(u, "uid") || !bodyField(n, "name")) {
    sendJson(400, "{\"message\":\"missing fields\"}"); return;
  }
  String code = authGenerateCode(u.to<String>(), n.to<String>());
  sendJson(201, "{\"code\":\"" + code + "\"}");
}

static void handleMyOrders() {
  String uid = authServer.arg("uid");
  if (!uid.length()) { sendJson(400, "{\"message\":\"missing uid\"}"); return; }
  sendJson(200, fbListOrders(uid));
}

static void handleAllOrders() {
  sendJson(200, fbListOrders(""));    // no filter -> all orders (admin)
}

static void handleDeleteOrder() {
  FirebaseJsonData o;
  if (!bodyField(o, "orderId")) { sendJson(400, "{\"message\":\"missing orderId\"}"); return; }
  bool ok = fbDeleteOrder(o.to<String>());
  sendJson(ok ? 200 : 500, ok ? "{}" : "{\"message\":\"delete failed\"}");
}

// Runs on Core 0, separate from the main state machine on Core 1, so it keeps
// responding even in the middle of a braid session.
static void webServerTask(void* param) {
  for (;;) {
    authServer.handleClient();
    delay(2);
  }
}

void webServerSetup() {
  authServer.on("/register",      HTTP_POST,    handleRegister);
  authServer.on("/login",         HTTP_POST,    handleLogin);
  authServer.on("/generate-code", HTTP_POST,    handleGenerateCode);
  authServer.on("/my-orders",     HTTP_GET,     handleMyOrders);
  authServer.on("/all-orders",    HTTP_GET,     handleAllOrders);
  authServer.on("/delete-order",  HTTP_POST,    handleDeleteOrder);
  authServer.on("/register",      HTTP_OPTIONS, handleCors);
  authServer.on("/login",         HTTP_OPTIONS, handleCors);
  authServer.on("/generate-code", HTTP_OPTIONS, handleCors);
  authServer.on("/delete-order",  HTTP_OPTIONS, handleCors);
  authServer.begin();
  xTaskCreatePinnedToCore(webServerTask, "webServer", 8192, NULL, 1, NULL, 0);
  Serial.println("[SECOND] Web auth server on port 80 (running on Core 0)");
}
