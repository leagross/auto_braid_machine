import { createContext, useContext, useState } from "react";
import { api } from "../api";

const AuthContext = createContext(null);

export function AuthProvider({ children }) {
  const [user, setUser] = useState(() => {
    const saved = localStorage.getItem("autobraid_user");
    return saved ? JSON.parse(saved) : null;
  });

  async function login(email, password) {
    const data = await api.login(email, password);
    const u = { email, name: data.name, role: data.role, uid: data.uid };
    setUser(u);
    localStorage.setItem("autobraid_user", JSON.stringify(u));
    return u;
  }

  async function register(email, password, name, role) {
    const data = await api.register(email, password, name, role);
    const u = { email, name, role, uid: data.uid };
    setUser(u);
    localStorage.setItem("autobraid_user", JSON.stringify(u));
    return u;
  }

  function logout() {
    api.logout().catch(() => {});
    setUser(null);
    localStorage.removeItem("autobraid_user");
  }

  return (
    <AuthContext.Provider value={{ user, login, register, logout }}>
      {children}
    </AuthContext.Provider>
  );
}

export function useAuth() {
  return useContext(AuthContext);
}
