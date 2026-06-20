import { useState } from "react";
import { signInWithEmailAndPassword, createUserWithEmailAndPassword } from "firebase/auth";
import { ref, set } from "firebase/database";
import { auth, db } from "../firebase";
import { Card, CardContent, CardDescription, CardFooter, CardHeader, CardTitle } from "@/components/ui/card";
import { Input } from "@/components/ui/input";
import { Button } from "@/components/ui/button";
import { Mail, Lock, User, Shield, Loader2 } from "lucide-react";

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
        const userCredential = await createUserWithEmailAndPassword(auth, email, password);
        const user = userCredential.user;
        
        await set(ref(db, `users/${user.uid}`), {
          name: name || "Người dùng LapGuard",
          email: email,
          created_at: Math.floor(Date.now() / 1000)
        });
      } else {
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
      } else if (
        err.code === "auth/invalid-credential" || 
        err.code === "auth/wrong-password" || 
        err.code === "auth/user-not-found"
      ) {
        errMsg = "Email hoặc mật khẩu không chính xác.";
      }
      setError(errMsg);
    } finally {
      setLoading(false);
    }
  };

  return (
    <div className="flex items-center justify-center min-h-[75vh] px-4">
      <Card className="w-full max-w-[420px] bg-white/80 dark:bg-card/90 backdrop-blur-lg border-border/50 shadow-2xl dynamic-card">
        <CardHeader className="space-y-1 text-center">
          <div className="flex justify-center mb-2">
            <div className="p-3 rounded-full bg-primary/10 text-primary">
              <Shield className="h-8 w-8 text-primary" />
            </div>
          </div>
          <CardTitle className="text-2xl font-bold tracking-tight">
            {isSignUp ? "Đăng ký LapGuard" : "Đăng nhập LapGuard"}
          </CardTitle>
          <CardDescription className="text-muted-foreground">
            {isSignUp 
              ? "Tạo tài khoản để giám sát thiết bị của bạn" 
              : "Hệ thống bảo vệ laptop thông minh thời gian thực"}
          </CardDescription>
        </CardHeader>
        <form onSubmit={handleSubmit}>
          <CardContent className="space-y-4">
            {isSignUp && (
              <div className="space-y-2">
                <label className="text-sm font-medium leading-none text-muted-foreground flex items-center gap-1.5">
                  <User className="h-3.5 w-3.5" /> Họ và tên
                </label>
                <Input
                  type="text"
                  placeholder="Nguyễn Văn A"
                  value={name}
                  onChange={(e) => setName(e.target.value)}
                  className="bg-black/5 dark:bg-black/20 border-border/50 focus-visible:bg-white dark:focus-visible:bg-background"
                  required={isSignUp}
                />
              </div>
            )}

            <div className="space-y-2">
              <label className="text-sm font-medium leading-none text-muted-foreground flex items-center gap-1.5">
                <Mail className="h-3.5 w-3.5" /> Email
              </label>
              <Input
                type="email"
                placeholder="email@example.com"
                value={email}
                onChange={(e) => setEmail(e.target.value)}
                className="bg-black/5 dark:bg-black/20 border-border/50 focus-visible:bg-white dark:focus-visible:bg-background"
                required
              />
            </div>

            <div className="space-y-2">
              <label className="text-sm font-medium leading-none text-muted-foreground flex items-center gap-1.5">
                <Lock className="h-3.5 w-3.5" /> Mật khẩu
              </label>
              <Input
                type="password"
                placeholder="••••••••"
                value={password}
                onChange={(e) => setPassword(e.target.value)}
                className="bg-black/5 dark:bg-black/20 border-border/50 focus-visible:bg-white dark:focus-visible:bg-background"
                required
              />
            </div>

            {error && (
              <p className="text-sm font-medium text-destructive mt-2">{error}</p>
            )}
          </CardContent>
          <CardFooter className="flex flex-col space-y-4">
            <Button 
              type="submit" 
              className="w-full bg-primary hover:bg-primary/90 text-primary-foreground font-semibold shadow-md dynamic-button"
              disabled={loading}
            >
              {loading ? (
                <>
                  <Loader2 className="mr-2 h-4 w-4 animate-spin" />
                  Đang xử lý...
                </>
              ) : (
                isSignUp ? "ĐĂNG KÝ" : "ĐĂNG NHẬP"
              )}
            </Button>
            
            <p className="text-sm text-center text-muted-foreground">
              {isSignUp ? "Đã có tài khoản? " : "Chưa có tài khoản? "}
              <span 
                onClick={() => { setIsSignUp(!isSignUp); setError(""); }}
                className="text-primary hover:underline cursor-pointer font-semibold"
              >
                {isSignUp ? "Đăng nhập" : "Đăng ký"}
              </span>
            </p>
          </CardFooter>
        </form>
      </Card>
    </div>
  );
}
