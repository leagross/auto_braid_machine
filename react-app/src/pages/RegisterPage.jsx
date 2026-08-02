import { useState } from "react";
import { useNavigate, Link } from "react-router-dom";
import { useAuth } from "../context/AuthContext";
import { api } from "../api";
import AuthCard  from "../components/AuthCard";
import FormField from "../components/FormField";
import styles    from "./AuthPage.module.css";

export default function RegisterPage() {
  const { register } = useAuth();
  const navigate     = useNavigate();

  const [form,    setForm]    = useState({ name: "", email: "", password: "", confirm: "" });
  const [errors,  setErrors]  = useState({});
  const [apiErr,  setApiErr]  = useState("");
  const [loading, setLoading] = useState(false);

  function validate() {
    const e = {};
    if (!form.name.trim())              e.name     = "שם הכרחי";
    if (!form.email.includes("@"))      e.email    = "כתובת מייל לא תקינה";
    if (form.password.length < 6)       e.password = "סיסמה חייבת להכיל לפחות 6 תווים";
    if (form.password !== form.confirm) e.confirm  = "הסיסמאות אינן תואמות";
    return e;
  }

  async function handleSubmit(e) {
    e.preventDefault();
    const e2 = validate();
    if (Object.keys(e2).length) { setErrors(e2); return; }
    setErrors({});
    setApiErr("");
    setLoading(true);
    try {
      const user = await register(form.email, form.password, form.name, "user");
      // יוצר קוד זמני מיד עם ההרשמה, כדי שהמשתמש לא יצטרך לבקש אותו בנפרד
      const code = await api.generateCode(user.uid, user.name);
      navigate("/dashboard", { state: { code } });
    } catch (err) {
      setApiErr(err.message);
    } finally {
      setLoading(false);
    }
  }

  return (
    <AuthCard title="הרשמה" subtitle="צור חשבון משתמש חדש">
      <form onSubmit={handleSubmit} noValidate>
        <FormField
          label="שם מלא"
          type="text"
          placeholder="ישראל ישראלי"
          value={form.name}
          onChange={e => setForm(f => ({ ...f, name: e.target.value }))}
          error={errors.name}
        />
        <FormField
          label="אימייל"
          type="email"
          placeholder="user@example.com"
          value={form.email}
          onChange={e => setForm(f => ({ ...f, email: e.target.value }))}
          error={errors.email}
          dir="ltr"
        />
        <FormField
          label="סיסמה"
          type="password"
          placeholder="לפחות 6 תווים"
          value={form.password}
          onChange={e => setForm(f => ({ ...f, password: e.target.value }))}
          error={errors.password}
          dir="ltr"
        />
        <FormField
          label="אימות סיסמה"
          type="password"
          placeholder="הזן שוב את הסיסמה"
          value={form.confirm}
          onChange={e => setForm(f => ({ ...f, confirm: e.target.value }))}
          error={errors.confirm}
          dir="ltr"
        />

        {apiErr && <div className={styles.apiError}>{apiErr}</div>}

        <button className={styles.btn} type="submit" disabled={loading}>
          {loading ? <span className={styles.spinner} /> : "הרשמה"}
        </button>
      </form>

      <p className={styles.switchLink}>
        יש לך חשבון?{" "}
        <Link to="/login">כניסה</Link>
      </p>
    </AuthCard>
  );
}
