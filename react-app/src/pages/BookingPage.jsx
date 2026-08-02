import { useState } from "react";
import { useNavigate } from "react-router-dom";
import { useAuth } from "../context/AuthContext";
import { api } from "../api";
import styles from "./BookingPage.module.css";

const HAIR_COLORS = [
  { id: "black",       label: "שחור",      hex: "#1a1a1a" },
  { id: "dark-brown",  label: "חום כהה",   hex: "#3b1f0e" },
  { id: "brown",       label: "חום",       hex: "#7b4a2d" },
  { id: "light-brown", label: "חום בהיר",  hex: "#b07d4a" },
  { id: "blonde",      label: "בלונד",     hex: "#d4b483" },
  { id: "platinum",    label: "פלטינום",   hex: "#e8e0c8" },
  { id: "red",         label: "אדום",      hex: "#b22222" },
  { id: "auburn",      label: "ערמוני",    hex: "#922b21" },
  { id: "gray",        label: "אפור",      hex: "#909090" },
  { id: "white",       label: "לבן",       hex: "#f0f0f0" },
  { id: "pink",        label: "ורוד",      hex: "#e75480" },
  { id: "blue",        label: "כחול",      hex: "#1e5fa8" },
];

export default function BookingPage() {
  const { user } = useAuth();
  const navigate  = useNavigate();

  const [extMode, setExtMode]         = useState("");
  const [selectedColors, setSelected] = useState([]);
  const [notes, setNotes]             = useState("");
  const [loading, setLoading]         = useState(false);
  const [done, setDone]               = useState(false);
  const [error, setError]             = useState("");

  function toggleColor(id) {
    setSelected(prev =>
      prev.includes(id) ? prev.filter(c => c !== id)
      : prev.length >= 3 ? prev
      : [...prev, id]
    );
  }

  async function handleSubmit() {
    setLoading(true);
    setError("");
    try {
      const extensions = extMode === "match"
        ? "כצבע השיער שלי"
        : selectedColors.map(id => HAIR_COLORS.find(c => c.id === id).label).join(", ");

      await api.createAppointment(user.uid, {
        userName:  user.name || user.email,
        extensions,
        notes,
        status: "בהמתנה",
      });
      setDone(true);
    } catch (e) {
      setError(e.message);
    } finally {
      setLoading(false);
    }
  }

  const canSubmit = extMode === "match" || (extMode === "manual" && selectedColors.length > 0);

  if (done) return (
    <div className={styles.page}>
      <div className={styles.successCard}>
        <div className={styles.successIcon}>🎉</div>
        <h2>ההזמנה נשמרה!</h2>
        <p className={styles.successSub}>ניתן להפעיל אותה מדף ההזמנות שלי</p>
        <div className={styles.successBtns}>
          <button className={styles.btnPrimary} onClick={() => navigate("/my-appointments")}>ההזמנות שלי</button>
          <button className={styles.btnOutline} onClick={() => navigate("/dashboard")}>לדף הבית</button>
        </div>
      </div>
    </div>
  );

  return (
    <div className={styles.page}>
      <header className={styles.header}>
        <button className={styles.back} onClick={() => navigate("/dashboard")}>← חזרה</button>
        <h1 className={styles.title}>הזמנה חדשה</h1>
      </header>

      <main className={styles.main}>
        <section className={styles.section}>

          {/* תוספות שיער */}
          <h2 className={styles.sectionTitle}>💇 תוספות שיער</h2>
          <p className={styles.sub}>בחרי עד 3 צבעים</p>

          <div className={styles.extModes}>
            <button
              className={`${styles.modeBtn} ${extMode === "match" ? styles.modeSelected : ""}`}
              onClick={() => { setExtMode("match"); setSelected([]); }}
            >
              <span>🎨</span><span>כצבע השיער שלי</span>
            </button>
            <button
              className={`${styles.modeBtn} ${extMode === "manual" ? styles.modeSelected : ""}`}
              onClick={() => setExtMode("manual")}
            >
              <span>🖌️</span><span>בחירה ידנית</span>
            </button>
          </div>

          {extMode === "manual" && (
            <>
              <div className={styles.colorCount}>נבחרו {selectedColors.length}/3</div>
              <div className={styles.colorGrid}>
                {HAIR_COLORS.map(c => {
                  const chosen = selectedColors.includes(c.id);
                  const maxed  = !chosen && selectedColors.length >= 3;
                  return (
                    <button
                      key={c.id}
                      className={`${styles.colorBtn} ${chosen ? styles.colorSelected : ""} ${maxed ? styles.colorDisabled : ""}`}
                      onClick={() => !maxed && toggleColor(c.id)}
                    >
                      <span className={styles.colorSwatch} style={{ background: c.hex }} />
                      <span className={styles.colorLabel}>{c.label}</span>
                      {chosen && <span className={styles.colorCheck}>✓</span>}
                    </button>
                  );
                })}
              </div>
            </>
          )}

          {/* הערות */}
          <label className={styles.label}>📝 הערות (אופציונלי)</label>
          <textarea
            className={styles.textarea}
            placeholder="בקשות מיוחדות, אורך רצוי..."
            value={notes}
            onChange={e => setNotes(e.target.value)}
            rows={3}
          />

          {error && <div className={styles.error}>{error}</div>}

          <button className={styles.btnPrimary} onClick={handleSubmit} disabled={loading || !canSubmit}>
            {loading ? "שומרת..." : "📩 שמירת הזמנה"}
          </button>
        </section>
      </main>
    </div>
  );
}
