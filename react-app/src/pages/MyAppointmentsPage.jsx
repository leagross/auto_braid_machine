import { useState, useEffect } from "react";
import { useNavigate } from "react-router-dom";
import { useAuth } from "../context/AuthContext";
import { api } from "../api";
import styles from "./MyAppointmentsPage.module.css";

// עמוד היסטוריית הזמנות — קורא מ-orders/{uid} שהמכונה (ESP32) כתבה בסיום קליעה.
export default function MyAppointmentsPage() {
  const { user }  = useAuth();
  const navigate  = useNavigate();

  const [orders,  setOrders]  = useState([]);
  const [loading, setLoading] = useState(true);
  const [error,   setError]   = useState("");
  const [deletingId, setDeletingId] = useState("");

  useEffect(() => {
    setLoading(true);
    api.getMyOrders(user.uid)
      .then(setOrders)
      .catch(e => setError(e.message))
      .finally(() => setLoading(false));
  }, [user.uid]);

  async function handleDelete(orderId) {
    if (!window.confirm("למחוק את ההזמנה הזו מההיסטוריה? הפעולה בלתי הפיכה.")) return;
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
        <h1 className={styles.title}>ההזמנות שלי</h1>
      </header>

      <main className={styles.main}>
        {loading && <div className={styles.loading}>טוענת...</div>}
        {error   && <div className={styles.error}>{error}</div>}

        {!loading && orders.length === 0 && (
          <div className={styles.empty}>
            <div className={styles.emptyIcon}>🪢</div>
            <p>אין הזמנות עדיין</p>
            <p className={styles.emptySub}>ההזמנות יופיעו כאן בסיום קליעה במכונה</p>
          </div>
        )}

        <div className={styles.list}>
          {orders.map((o) => {
            const isEmergency = o.status === "emergency";
            return (
              <div key={o.id} className={styles.card}>
                <div className={styles.cardTop}>
                  {isEmergency ? (
                    <span className={styles.status} style={{ color: "#ff5c6c", background: "rgba(255,92,108,.12)" }}>
                      עצירת חירום ⛔
                    </span>
                  ) : (
                    <span className={styles.status} style={{ color: "#3ecf8e", background: "rgba(62,207,142,.12)" }}>
                      נעשה ✓
                    </span>
                  )}
                  <span className={styles.cardDate}>
                    {o.createdAt ? new Date(o.createdAt).toLocaleDateString("he-IL") : ""}
                  </span>
                </div>

                <div className={styles.cardRow}>
                  <span>💇</span>
                  <span>תוספות: {o.extensions || "ללא"}</span>
                </div>

                {o.hairColor && o.hairColor !== "-" && (
                  <div className={styles.cardRow}>
                    <span>🎨</span>
                    <span>צבע שיער שזוהה: {o.hairColor}</span>
                  </div>
                )}

                <button
                  className={styles.cancelBtn}
                  onClick={() => handleDelete(o.id)}
                  disabled={deletingId === o.id}
                >
                  {deletingId === o.id ? "מוחקת..." : "🗑 מחיקה מההיסטוריה"}
                </button>
              </div>
            );
          })}
        </div>
      </main>
    </div>
  );
}
