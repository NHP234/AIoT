import React, { useState, useEffect } from "react";
import { onAuthStateChanged, signOut } from "firebase/auth";
import { ref, onValue, off } from "firebase/database";
import { auth, db } from "./firebase";
import { Toaster } from "@/components/ui/sonner";
import { toast } from "sonner";
import { Button } from "@/components/ui/button";
import { Shield, LogOut, User, Loader2 } from "lucide-react";

import Auth from "./components/Auth";
import Dashboard from "./components/Dashboard";
import DeviceManager from "./components/DeviceManager";
import AlarmLogs from "./components/AlarmLogs";
import ThemeToggle from "./components/ThemeToggle";

export default function App() {
  const [user, setUser] = useState(null);
  const [loading, setLoading] = useState(true);
  const [devices, setDevices] = useState([]);
  const [activeDevice, setActiveDevice] = useState("");
  const [logs, setLogs] = useState([]);
  const [notificationPermission, setNotificationPermission] = useState("default");

  // Track Authentication State
  useEffect(() => {
    const unsubscribe = onAuthStateChanged(auth, (currentUser) => {
      setUser(currentUser);
      setLoading(false);
      if (!currentUser) {
        setDevices([]);
        setActiveDevice("");
        setLogs([]);
      }
    });

    return () => unsubscribe();
  }, []);

  // Request Notification Permission
  useEffect(() => {
    if ("Notification" in window) {
      setNotificationPermission(Notification.permission);
      if (Notification.permission === "default") {
        Notification.requestPermission().then((permission) => {
          setNotificationPermission(permission);
        });
      }
    }
  }, [user]);

  // Synchronize Devices and Logs for Authenticated User
  useEffect(() => {
    if (!user) return;

    const userDevicesRef = ref(db, `users/${user.uid}/devices`);
    const deviceListeners = {};

    const handleUserDevicesChange = (snapshot) => {
      const userDeviceMap = snapshot.val() || {};
      const macs = Object.keys(userDeviceMap);

      if (macs.length === 0) {
        setDevices([]);
        setActiveDevice("");
        return;
      }

      if (!activeDevice || !macs.includes(activeDevice)) {
        setActiveDevice(macs[0]);
      }

      macs.forEach((mac) => {
        if (!deviceListeners[mac]) {
          const deviceRef = ref(db, `devices/${mac}`);
          const listener = onValue(deviceRef, (devSnapshot) => {
            const devData = devSnapshot.val();
            if (devData) {
              setDevices((prevDevices) => {
                const filtered = prevDevices.filter((d) => d.id !== mac);
                return [...filtered, { id: mac, ...devData }].sort((a, b) => a.id.localeCompare(b.id));
              });

              // Check if device status changed to TRIGGERED, trigger local push notification & sonner toast alert
              if (devData.status === "TRIGGERED") {
                const lastAlertKey = `last_alert_${mac}`;
                const lastAlertTime = sessionStorage.getItem(lastAlertKey);
                const now = Date.now();
                
                if (!lastAlertTime || (now - parseInt(lastAlertTime)) > 10000) {
                  sessionStorage.setItem(lastAlertKey, now.toString());
                  
                  // 1. Sonner toast
                  toast.error(`🚨 CẢNH BÁO TRỘM: Thiết bị ${devData.device_name || mac} đang bị tác động!`, {
                    duration: 10000,
                  });

                  // 2. Native notification
                  if (Notification.permission === "granted") {
                    new Notification("🚨 CẢNH BÁO LAPGUARD!", {
                      body: `Phát hiện di chuyển bất thường trên thiết bị: ${devData.device_name || mac}!`,
                      icon: "/favicon.svg",
                      tag: `alarm-${mac}`,
                      requireInteraction: true,
                    });
                  }
                }
              }
            }
          });
          deviceListeners[mac] = { ref: deviceRef, callback: listener };
        }
      });

      Object.keys(deviceListeners).forEach((mac) => {
        if (!macs.includes(mac)) {
          off(deviceListeners[mac].ref, "value", deviceListeners[mac].callback);
          delete deviceListeners[mac];
          setDevices((prevDevices) => prevDevices.filter((d) => d.id !== mac));
        }
      });
    };

    onValue(userDevicesRef, handleUserDevicesChange);

    const logsRef = ref(db, "logs");
    const handleLogsChange = (snapshot) => {
      const logsMap = snapshot.val() || {};
      const logsList = Object.keys(logsMap).map((key) => ({
        id: key,
        ...logsMap[key],
      }));
      setLogs(logsList);
    };

    onValue(logsRef, handleLogsChange);

    return () => {
      off(userDevicesRef, "value", handleUserDevicesChange);
      off(logsRef, "value", handleLogsChange);
      Object.keys(deviceListeners).forEach((mac) => {
        off(deviceListeners[mac].ref, "value", deviceListeners[mac].callback);
      });
    };
  }, [user, activeDevice]);

  const handleLogout = async () => {
    try {
      await signOut(auth);
      toast.success("Đã đăng xuất thành công.");
    } catch (err) {
      console.error("Logout failed:", err);
      toast.error("Đăng xuất thất bại.");
    }
  };

  if (loading) {
    return (
      <div className="flex flex-col items-center justify-center min-h-screen space-y-4">
        <Loader2 className="h-10 w-10 text-cyan-400 animate-spin" />
        <span className="text-sm font-semibold tracking-wider text-muted-foreground uppercase">Đang khởi động hệ thống...</span>
      </div>
    );
  }

  if (!user) {
    return (
      <div className="app-container">
        <header className="flex justify-between items-center mb-8 pb-4 border-b border-border/10">
          <div className="flex items-center gap-2.5">
            <span className="text-2xl font-black tracking-tighter bg-gradient-to-r from-cyan-400 to-purple-500 bg-clip-text text-transparent">
              LapGuard
            </span>
            <span className="text-[10px] font-black uppercase bg-cyan-400/10 text-cyan-400 px-2 py-0.5 rounded border border-cyan-400/20 tracking-widest">
              IoT Security
            </span>
          </div>
          <ThemeToggle />
        </header>
        <Auth />
        <Toaster />
      </div>
    );
  }

  const activeDeviceData = devices.find((d) => d.id === activeDevice);
  const isTriggered = activeDeviceData && activeDeviceData.status === "TRIGGERED";

  return (
    <div className={`app-container min-h-screen ${isTriggered ? "app-triggered" : ""}`}>
      <header className="flex justify-between items-center mb-8 pb-4 border-b border-border/10">
        <div className="flex items-center gap-2.5">
          <span className="text-2xl font-black tracking-tighter bg-gradient-to-r from-cyan-400 to-purple-500 bg-clip-text text-transparent">
            LapGuard
          </span>
          <span className="text-[10px] font-black uppercase bg-cyan-400/10 text-cyan-400 px-2 py-0.5 rounded border border-cyan-400/20 tracking-widest">
            Dashboard
          </span>
        </div>
        <div className="flex items-center gap-4">
          <ThemeToggle />
          <div className="hidden md:flex items-center gap-1.5 text-sm text-muted-foreground bg-black/20 dark:bg-black/40 px-3.5 py-1.5 rounded-full border border-border/30">
            <User className="h-4 w-4 text-cyan-400" />
            <span>{user.email}</span>
          </div>
          <Button
            variant="outline"
            className="border-border/60 hover:bg-red-500/10 hover:text-red-500 hover:border-red-500 flex items-center gap-1.5 rounded-full transition-all"
            onClick={handleLogout}
          >
            <LogOut className="h-4 w-4" /> ĐĂNG XUẤT
          </Button>
        </div>
      </header>

      <div className="grid grid-cols-1 lg:grid-cols-3 gap-6">
        <div className="lg:col-span-2 space-y-6">
          <Dashboard activeDevice={activeDevice} deviceData={activeDeviceData} />
          {activeDevice && <AlarmLogs activeDevice={activeDevice} logs={logs} />}
        </div>
        <div className="lg:col-span-1">
          <DeviceManager
            user={user}
            devices={devices}
            activeDevice={activeDevice}
            setActiveDevice={setActiveDevice}
          />
        </div>
      </div>
      
      {/* Toast provider */}
      <Toaster />
    </div>
  );
}
