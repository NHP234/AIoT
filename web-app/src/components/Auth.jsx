import React, { useState } from "react";
import { signInWithEmailAndPassword, createUserWithEmailAndPassword } from "firebase/auth";
import { ref, set } from "firebase/database";
import { auth, db } from "../firebase";

export default function Auth() {
  const [isSignUp, setIsSignUp] = useState(false);
  const [email, setEmail] = useState("");
  const [password, setPassword] = useState("");
  const [name, setName] = useState("");
  const [error, setError] = useState("");
  const [loading, setLoading] = useState(false);

  const handleSubmit = async (e) => {
    e.preventDefault();
    setError("");
    setLoading(true);

    try {
      if (isSignUp) {
        // Sign Up Flow
        const userCredential = await createUserWithEmailAndPassword(auth, email, password);
        const user = userCredential.user;
        
        // Save user info to Realtime Database
        await set(ref(db, `users/${user.uid}`), {
          name: name || "Người dùng LapGuard",
          email: email,
          created_at: Math.floor(Date.now() / 1000)
        });
      } else {
        // Sign In Flow
        await signInWithEmailAndPassword(auth, email, password);
      }
    } catch (err) {
      console.error(err);
      let errMsg = "Đã xảy ra lỗi. Vui lòng thử lại.";
      if (err.code === "auth/email-already-in-use") {
        errMsg = "Email này đã được sử dụng.";
      } else if (err.code === "auth/invalid-email") {
        errMsg = "Địa chỉ email không hợp lệ.";
      } else if (err.code === "auth/weak-password") {
        errMsg = "Mật khẩu quá yếu (tối thiểu 6 ký tự).";
      } else if (err.code === "auth/invalid-credential" || err.code === "auth/wrong-password" || err.code === "auth/user-not-found") {
        errMsg = "Email hoặc mật khẩu không chính xác.";
      }
      setError(errMsg);
    } finally {
      setLoading(false);
    }
  };

  return (
    <div className="auth-container">
      <div className="glass-card auth-card">
        <h2 className="auth-title">{isSignUp ? "Đăng ký LapGuard" : "Đăng nhập LapGuard"}</h2>
        <p className="auth-subtitle">
          {isSignUp 
            ? "Tạo tài khoản để giám sát thiết bị của bạn" 
            : "Hệ thống bảo vệ laptop thông minh thời gian thực"}
        </p>

        <form onSubmit={handleSubmit}>
          {isSignUp && (
            <div className="form-group">
              <label className="form-label">Tên của bạn</label>
              <input
                type="text"
                className="form-input"
                placeholder="Nhập họ và tên"
                value={name}
                onChange={(e) => setName(e.target.value)}
                required={isSignUp}
              />
            </div>
          )}

          <div className="form-group">
            <label className="form-label">Email</label>
            <input
              type="email"
              className="form-input"
              placeholder="nhap-email@example.com"
              value={email}
              onChange={(e) => setEmail(e.target.value)}
              required
            />
          </div>

          <div className="form-group">
            <label className="form-label">Mật khẩu</label>
            <input
              type="password"
              className="form-input"
              placeholder="••••••••"
              value={password}
              onChange={(e) => setPassword(e.target.value)}
              required
            />
          </div>

          {error && <div className="error-message">{error}</div>}

          <button 
            type="submit" 
            className="btn btn-primary" 
            style={{ width: "100%", marginTop: "12px", padding: "12px" }}
            disabled={loading}
          >
            {loading ? "Đang xử lý..." : isSignUp ? "ĐĂNG KÝ NGAY" : "ĐĂNG NHẬP"}
          </button>
        </form>

        <div className="auth-toggle">
          {isSignUp ? (
            <>
              Đã có tài khoản?{" "}
              <span onClick={() => { setIsSignUp(false); setError(""); }}>Đăng nhập</span>
            </>
          ) : (
            <>
              Chưa có tài khoản?{" "}
              <span onClick={() => { setIsSignUp(true); setError(""); }}>Đăng ký</span>
            </>
          )}
        </div>
      </div>
    </div>
  );
}
