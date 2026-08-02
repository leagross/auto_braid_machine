import { useState, useEffect } from "react";
import { useNavigate } from "react-router-dom";
import { useAuth } from "../context/AuthContext";
import { api } from "../api";
import styles from "./AdminPage.module.css";

// עמוד המנהל — מציג את כל ההזמנות של כל הלקוחות (המכונה כותבת אותן).
// שתי לשוניות נפרדות: הזמנות שהושלמו בהצלחה, והזמנות שנעצרו בחירום.
export default function AdminPage() {
  const { user } = useAuth();
  const navigate  = useNavigate();

  const [orders,  setOrders]  = useState([]);
  const [loading, setLoading] = useState(true);
  const [error,   setError]   = useState("");
  const [search,  setSearch]  = useState("");
  const [tab,     setTab]     = useState("completed");  // "completed" | "emergency"
  const [deletingId, setDeletingId] = useState("");

  useEffect(() => {
    if (user.role !== "admin") { navigate("/dashboard"); return; }
    api.getAllOrders()
      .then(setOrders)
      .catch(e => setError(e.message))
      .finally(() => setLoading(false));
  }, []);

  // הזמנות ישנות (לפני שהוספנו status) ייחשבו כ"הושלמו" כברירת מחדל
  const completedOrders = orders.filter(o => o.status !== "emergency");
  const emergencyOrders = orders.filter(o => o.status === "emergency");
  const shown = tab === "emergency" ? emergencyOrders : completedOrders;

  const filtered = shown.filter(o =>
    !search ||
    o.name?.includes(search) ||
    o.extensions?.includes(search) ||
    o.hairColor?.includes(search)
  );

  const uniqueClients = new Set(orders.map(o => o.uid)).size;

  function formatDate(createdAt) {
    if (typeof createdAt === "number") return "—";           // millis מה-ESP32, לא תאריך אמיתי
    if (createdAt?.seconds) return new Date(createdAt.seconds * 1000).toLocaleDateString("he-IL");
    return "—";
  }

  async function handleDelete(orderId, clientName) {
    if (!window.confirm(`למחוק את ההזמנה של ${clientName || "הלקוח"} לצמיתות?`)) return;
    setDeletingId(orderId);
    try {
      await api.deleteOrder(orderId);
      setOrders(prev => prev.filter(o => o.id !== orderId));
    } catch (e) {
      setError(e.message);
    } finally {
      setDeletingId("");
    }
  }

  return (
    <div className={styles.page}>
      <header className={styles.header}>
        <button className={styles.back} onClick={() => navigate("/dashboard")}>← חזרה</button>
        <h1 className={styles.title}>👑 לוח מנהלת</h1>
      </header>

      <main className={styles.main}>
        {/* כרטיסי סיכום */}
        <div className={styles.statsRow} style={{ gridTemplateColumns: "repeat(3, 1fr)" }}>
          <div className={styles.statCard}>
            <span className={styles.statNum}>{completedOrders.length}</span>
            <span className={styles.statLabel}>הזמנות שהושלמו</span>
          </div>
          <div className={styles.statCard} style={{ borderColor: "#ff5c6c" }}>
            <span className={styles.statNum} style={{ color: "#ff5c6c" }}>{emergencyOrders.length}</span>
            <span className={styles.statLabel}>עצירות חירום</span>
          </div>
          <div className={styles.statCard}>
            <span className={styles.statNum}>{uniqueClients}</span>
            <span className={styles.statLabel}>לקוחות שונים</span>
          </div>
        </div>

        {/* לשוניות */}
        <div className={styles.filters}>
          <button
            className={`${styles.filterBtn} ${tab === "completed" ? styles.filterActive : ""}`}
            style={tab === "completed" ? { background: "#3ecf8e", borderColor: "#3ecf8e" } : {}}
            onClick={() => setTab("completed")}
          >✅ הזמנות ({completedOrders.length})</button>
          <button
            className={`${styles.filterBtn} ${tab === "emergency" ? styles.filterActive : ""}`}
            style={tab === "emergency" ? { background: "#ff5c6c", borderColor: "#ff5c6c" } : {}}
            onClick={() => setTab("emergency")}
          >⛔ עצירות חירום ({emergencyOrders.length})</button>
        </div>

        {/* חיפוש */}
        <input
          className={styles.search}
          placeholder="🔍 חיפוש לפי שם לקוח, תוספות, צבע..."
          value={search}
          onChange={e => setSearch(e.target.value)}
        />

        {loading && <div className={styles.loading}>טוענת נתונים...</div>}
        {error   && <div className={styles.error}>{error}</div>}
        {!loading && filtered.length === 0 && (
          <div className={styles.empty}>
            {tab === "emergency" ? "אין עצירות חירום להצגה" : "אין הזמנות להצגה"}
          </div>
        )}

        {/* רשימת הזמנות */}
        <div className={styles.list}>
          {filtered.map((o) => (
            <div
              key={o.id}
              className={`${styles.card} ${tab === "emergency" ? "" : styles.cardDone}`}
              style={tab === "emergency" ? { borderColor: "rgba(255,92,108,.3)" } : {}}
            >
              <div className={styles.cardHeader}>
                <div className={styles.clientInfo}>
                  <span className={styles.clientName}>{o.name || "—"}</span>
                  <span className={styles.clientDate}>📅 {formatDate(o.createdAt)}</span>
                </div>
                {tab === "emergency" ? (
                  <span className={styles.statusBadge} style={{ color: "#ff5c6c", background: "rgba(255,92,108,.12)" }}>
                    עצירת חירום ⛔
                  </span>
                ) : (
                  <span className={styles.statusBadge} style={{ color: "#3ecf8e", background: "rgba(62,207,142,.12)" }}>
                    נעשה ✓
                  </span>
                )}
              </div>

              <div className={styles.cardBody}>
                <div className={styles.detailRow}>
                  <span className={styles.detailIcon}>💇</span>
                  <span className={styles.detailVal}>תוספות: {o.extensions || "ללא"}</span>
                </div>
                {o.hairColor && o.hairColor !== "-" && (
                  <div className={styles.detailRow}>
                    <span className={styles.detailIcon}>🎨</span>
                    <span className={styles.detailVal}>צבע שיער שזוהה: {o.hairColor}</span>
                  </div>
                )}
              </div>

              <button
                style={{
                  margin: "0 16px 14px", padding: "9px",
                  background: "transparent", color: "#ff5c6c",
                  border: "1.5px solid #ff5c6c", borderRadius: 10,
                  fontSize: 13, fontWeight: 600, cursor: "pointer",
                  opacity: deletingId === o.id ? 0.5 : 1,
                }}
                onClick={() => handleDelete(o.id, o.name)}
                disabled={deletingId === o.id}
              >
                {deletingId === o.id ? "מוחקת..." : "🗑 מחיקה"}
              </button>
            </div>
          ))}
        </div>
      </main>
    </div>
  );
}
