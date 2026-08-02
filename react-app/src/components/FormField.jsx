import styles from "./FormField.module.css";

export default function FormField({ label, error, ...props }) {
  return (
    <div className={styles.group}>
      <label className={styles.label}>{label}</label>
      <input className={`${styles.input} ${error ? styles.inputError : ""}`} {...props} />
      {error && <span className={styles.error}>{error}</span>}
    </div>
  );
}
