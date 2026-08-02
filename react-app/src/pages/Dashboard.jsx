import { useState } from "react";
import { useNavigate, useLocation } from "react-router-dom";
import { useAuth } from "../context/AuthContext";
import { api } from "../api";
import styles from "./Dashboard.module.css";

export default function Dashboard() {
  const { user, logout } = useAuth();
  const navigate = useNavigate();
  const location = useLocation();

  // אם הגענו מיד אחרי הרשמה, הקוד כבר נוצר שם ומגיע דרך ה-state
  const [code, setCode]       = useState(location.state?.code || "");
  const [codeLoading, setCodeLoading] = useState(false);

  function handleLogout() {
    logout();
    navigate("/login");
  }

  async function handleGenerateCode() {
    setCodeLoading(true);
    try {
      const c = await api.generateCode(user.uid, user.name || user.email);
      setCode(c);
    } catch (e) {
      alert("שגיאה ביצירת קוד: " + e.message);
    } finally {
      setCodeLoading(false);
    }
  }

  const isAdmin = user.role === "admin";

  return (
    <div className={styles.page}>
      <header className={styles.header}>
        <div className={styles.logo}>🌀 AutoBraid</div>
        <div className={styles.userInfo}>
          <span className={`${styles.badge} ${isAdmin ? styles.admin : styles.user}`}>
            {isAdmin ? "👑 מנהלת" : "👤 לקוחה"}
          </span>
          <span className={styles.name}>{user.name || user.email}</span>
          <button className={styles.logoutBtn} onClick={handleLogout}>יציאה</button>
        </div>
      </header>

      <main className={styles.main}>
        <div className={styles.heroSection}>
          <div className={styles.heroEmoji}>🪢</div>
          <h1 className={styles.welcome}>שלום, {user.name || "משתמשת"} 👋</h1>
          <p className={styles.sub}>
            {isAdmin ? "ברוכה הבאה לממשק הניהול" : "מה תרצי לעשות היום?"}
          </p>
        </div>

        {/* כפתורי פעולה ראשיים */}
        {!isAdmin && (
          <div className={styles.actionCards}>
            <button className={styles.actionCard} onClick={handleGenerateCode} disabled={codeLoading}>
              <span className={styles.actionEmoji}>🔢</span>
              <span className={styles.actionTitle}>{codeLoading ? "יוצר..." : "קוד למכונה"}</span>
              <span className={styles.actionDesc}>צרי קוד זמני להקלדה על המסך</span>
            </button>
            <button className={styles.actionCard} onClick={() => navigate("/my-appointments")}>
              <span className={styles.actionEmoji}>📋</span>
              <span className={styles.actionTitle}>ההזמנות שלי</span>
              <span className={styles.actionDesc}>היסטוריית ההזמנות שלך</span>
            </button>
          </div>
        )}

        {/* הצגת הקוד הזמני שנוצר */}
        {code && (
          <div className={styles.heroSection}>
            <p className={styles.sub}>הקוד הזמני שלך (הקלידי אותו על מסך המכונה):</p>
            <div style={{ fontSize: 48, fontWeight: 800, letterSpacing: 12, color: "#6c63ff" }}>
              {code}
            </div>
          </div>
        )}

        {isAdmin && (
          <div className={styles.actionCards}>
            <button className={styles.actionCard} onClick={() => navigate("/admin")}>
              <span className={styles.actionEmoji}>📊</span>
              <span className={styles.actionTitle}>כל ההזמנות</span>
              <span className={styles.actionDesc}>ניהול תורים ולקוחות</span>
            </button>
          </div>
        )}

        {/* כרטיסי מצב */}
        <div className={styles.cards}>
          <div className={styles.card}>
            <div className={styles.cardIcon}>📡</div>
            <div className={styles.cardTitle}>ESP32</div>
            <div className={styles.cardValue}>פעיל</div>
          </div>
          <div className={styles.card}>
            <div className={styles.cardIcon}>🔥</div>
            <div className={styles.cardTitle}>Firebase</div>
            <div className={styles.cardValue}>מחובר</div>
          </div>
          <div className={styles.card}>
            <div className={styles.cardIcon}>🔑</div>
            <div className={styles.cardTitle}>הרשאה</div>
            <div className={styles.cardValue}>{isAdmin ? "מנהלת" : "לקוחה"}</div>
          </div>
        </div>
      </main>
    </div>
  );
}
