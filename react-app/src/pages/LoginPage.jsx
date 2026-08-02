import { useState } from "react";
import { useNavigate, Link } from "react-router-dom";
import { useAuth } from "../context/AuthContext";
import AuthCard  from "../components/AuthCard";
import FormField from "../components/FormField";
import styles    from "./AuthPage.module.css";

export default function LoginPage() {
  const { login } = useAuth();
  const navigate  = useNavigate();

  const [form,    setForm]    = useState({ email: "", password: "" });
  const [errors,  setErrors]  = useState({});
  const [apiErr,  setApiErr]  = useState("");
  const [loading, setLoading] = useState(false);

  function validate() {
    const e = {};
    if (!form.email.includes("@"))  e.email    = "כתובת מייל לא תקינה";
    if (form.password.length < 4)   e.password = "סיסמה קצרה מדי";
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
      const user = await login(form.email, form.password);
      navigate(user.role === "admin" ? "/dashboard" : "/dashboard");
    } catch (err) {
      setApiErr(err.message);
    } finally {
      setLoading(false);
    }
  }

  return (
    <AuthCard title="ברוך הבא" subtitle="התחבר למערכת AutoBraid">
      <form onSubmit={handleSubmit} noValidate>
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
          placeholder="••••••••"
          value={form.password}
          onChange={e => setForm(f => ({ ...f, password: e.target.value }))}
          error={errors.password}
          dir="ltr"
        />

        {apiErr && <div className={styles.apiError}>{apiErr}</div>}

        <button className={styles.btn} type="submit" disabled={loading}>
          {loading ? <span className={styles.spinner} /> : "כניסה"}
        </button>
      </form>

      <p className={styles.switchLink}>
        אין לך חשבון?{" "}
        <Link to="/register">הרשמה</Link>
      </p>
    </AuthCard>
  );
}
