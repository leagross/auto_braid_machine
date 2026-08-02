import styles from "./AuthCard.module.css";

export default function AuthCard({ title, subtitle, children }) {
  return (
    <div className={styles.wrapper}>
      <div className={styles.glow} />
      <div className={styles.card}>
        <div className={styles.logo}>
          <span className={styles.logoIcon}>⚙</span>
          <span className={styles.logoText}>AutoBraid</span>
        </div>
        <h1 className={styles.title}>{title}</h1>
        {subtitle && <p className={styles.subtitle}>{subtitle}</p>}
        {children}
      </div>
    </div>
  );
}
