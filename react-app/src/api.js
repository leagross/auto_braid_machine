// ============================================================================
//  api.js  —  שכבת הנתונים של האפליקציה מעל Firebase (Auth + Cloud Firestore)
//  ----------------------------------------------------------------------------
//  הזרימה:
//    1. המשתמש נרשם / מתחבר עם מייל וסיסמה  (Firebase Auth).
//    2. המערכת מייצרת קוד זמני ומאחסנת אותו במסמך codes/{code}.
//    3. המשתמש מקליד את הקוד על מסך המכונה (ESP32 השני), שמאמת מול אותו מסמך.
//    4. בסיום, המכונה כותבת מסמך הזמנה חדש ב-orders/{id} — כאן קוראים אותו כהיסטוריה.
//
//  ℹ️ תפקיד "מנהל": אין מסך הרשמה למנהל בכוונה (מטעמי אבטחה). כדי להפוך
//     משתמש קיים למנהל — ערכי ידנית ב-Firebase Console -> Firestore ->
//     users/{uid} -> שדה role -> "admin".
// ============================================================================
// ה-ESP32 שומר createdAt כמספר millis() (זמן מאז הדלקת הלוח, לא תאריך אמיתי),
// והאפליקציה שומרת Firestore Timestamp (עם שדה .seconds). ממיר את שניהם למספר להשוואה.
export function toSortMs(createdAt) {
  if (!createdAt) return 0;
  if (typeof createdAt === "number") return createdAt;      // מגיע מה-ESP32
  if (createdAt.seconds) return createdAt.seconds * 1000;    // Firestore Timestamp
  return 0;
}

// כתובת ה-ESP32 השני ברשת המקומית (עדכני לפי ה-IP שמודפס ב-Serial Monitor בהדלקה)
const ESP32_URL = "http://192.168.0.101";

async function esp32Post(path, payload) {
  const res = await fetch(ESP32_URL + path, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(payload),
  });
  const data = await res.json();
  if (!res.ok) throw new Error(data.message || "שגיאה בתקשורת עם המכשיר");
  return data;
}

async function esp32Get(path) {
  const res = await fetch(ESP32_URL + path);
  const data = await res.json();
  if (!res.ok) throw new Error((data && data.message) || "שגיאה בתקשורת עם המכשיר");
  return data;
}

export const api = {
  // ---- אימות: כל הבקשה עוברת דרך ה-ESP32 (לא ישירות ל-Firestore) ----
  async register(email, password, name) {
    return esp32Post("/register", { email, password, name });   // {uid, name, role}
  },

  async login(email, password) {
    return esp32Post("/login", { email, password });             // {uid, name, role}
  },

  async logout() {
    // אין session בצד השרת לנקות — רק ניקוי מקומי (localStorage/state) בצד הלקוח.
  },

  // ---- קוד זמני למכונה: גם זה נוצר ע"י ה-ESP32, לא ע"י האפליקציה ----
  async generateCode(uid, name) {
    const { code } = await esp32Post("/generate-code", { uid, name });
    return code;
  },

  // ---- היסטוריית הזמנות: גם זה עובר דרך ה-ESP32 (לא ישירות ל-Firestore) ----
  async getMyOrders(uid) {
    const orders = await esp32Get("/my-orders?uid=" + encodeURIComponent(uid));
    return orders.sort((a, b) => toSortMs(b.createdAt) - toSortMs(a.createdAt));
  },

  // ---- מנהל: כל ההזמנות של כולם ----
  async getAllOrders() {
    const orders = await esp32Get("/all-orders");
    return orders.sort((a, b) => toSortMs(b.createdAt) - toSortMs(a.createdAt));
  },

  // ---- מחיקת הזמנה בודדת מההיסטוריה (לקוח או מנהל) ----
  async deleteOrder(orderId) {
    await esp32Post("/delete-order", { orderId });
  },
};
