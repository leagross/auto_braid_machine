import { BrowserRouter, Routes, Route, Navigate } from "react-router-dom";
import LoginPage          from "./pages/LoginPage";
import RegisterPage       from "./pages/RegisterPage";
import Dashboard          from "./pages/Dashboard";
import MyAppointmentsPage from "./pages/MyAppointmentsPage";
import AdminPage          from "./pages/AdminPage";
import { AuthProvider, useAuth } from "./context/AuthContext";

function PrivateRoute({ children, adminOnly = false }) {
  const { user } = useAuth();
  if (!user) return <Navigate to="/login" replace />;
  if (adminOnly && user.role !== "admin") return <Navigate to="/dashboard" replace />;
  return children;
}

export default function App() {
  return (
    <AuthProvider>
      <BrowserRouter>
        <Routes>
          <Route path="/login"            element={<LoginPage />} />
          <Route path="/register"         element={<RegisterPage />} />
          <Route path="/dashboard"        element={<PrivateRoute><Dashboard /></PrivateRoute>} />
          <Route path="/my-appointments"  element={<PrivateRoute><MyAppointmentsPage /></PrivateRoute>} />
          <Route path="/admin"            element={<PrivateRoute adminOnly><AdminPage /></PrivateRoute>} />
          <Route path="*"                 element={<Navigate to="/login" replace />} />
        </Routes>
      </BrowserRouter>
    </AuthProvider>
  );
}
