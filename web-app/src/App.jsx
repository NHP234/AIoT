import React, { useState, useEffect } from "react";
import { onAuthStateChanged, signOut } from "firebase/auth";
import { ref, onValue, off } from "firebase/database";
import { auth, db } from "./firebase";

import Auth from "./components/Auth";
import Dashboard from "./components/Dashboard";
import DeviceManager from "./components/DeviceManager";
import AlarmLogs from "./components/AlarmLogs";

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
        // Reset state on logout
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

    // Listen to user's device mappings: /users/$uid/devices
    const userDevicesRef = ref(db, `users/${user.uid}/devices`);
    
    // Dictionary to hold listener references to clean them up
    const deviceListeners = {};

    const handleUserDevicesChange = (snapshot) => {
      const userDeviceMap = snapshot.val() || {};
      const macs = Object.keys(userDeviceMap);

      if (macs.length === 0) {
        setDevices([]);
        setActiveDevice("");
        return;
      }

      // Set default active device if none selected
      if (!activeDevice || !macs.includes(activeDevice)) {
        setActiveDevice(macs[0]);
      }

      // Set up listeners for individual devices
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

              // Check if device status changed to TRIGGERED, trigger local push notification
              if (devData.status === "TRIGGERED" && Notification.permission === "granted") {
                // Throttle notifications to prevent spamming
                const lastAlertKey = `last_alert_${mac}`;
                const lastAlertTime = sessionStorage.getItem(lastAlertKey);
                const now = Date.now();
                
                if (!lastAlertTime || (now - parseInt(lastAlertTime)) > 10000) {
                  sessionStorage.setItem(lastAlertKey, now.toString());
                  new Notification("🚨 CẢNH BÁO LAPGUARD!", {
                    body: `Phát hiện di chuyển bất thường trên thiết bị: ${devData.device_name || mac}!`,
                    icon: "/favicon.svg",
                    tag: `alarm-${mac}`,
                    requireInteraction: true,
                  });
                }
              }
            }
          });
          deviceListeners[mac] = { ref: deviceRef, callback: listener };
        }
      });

      // Clean up orphaned listeners if devices were unlinked
      Object.keys(deviceListeners).forEach((mac) => {
        if (!macs.includes(mac)) {
          off(deviceListeners[mac].ref, "value", deviceListeners[mac].callback);
          delete deviceListeners[mac];
          setDevices((prevDevices) => prevDevices.filter((d) => d.id !== mac));
        }
      });
    };

    onValue(userDevicesRef, handleUserDevicesChange);

    // Listen to /logs node
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

    // Cleanup listeners
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
    } catch (err) {
      console.error("Logout failed:", err);
    }
  };

  if (loading) {
    return (
      <div className="loading-container">
        <div className="loading-spinner"></div>
        <span>Đang khởi động hệ thống...</span>
      </div>
    );
  }

  if (!user) {
    return (
      <div className="app-container">
        <header className="app-header" style={{ justifyContent: "center" }}>
          <div className="brand">
            <span className="brand-logo">LapGuard</span>
            <span className="brand-tag">IoT Security</span>
          </div>
        </header>
        <Auth />
      </div>
    );
  }

  const activeDeviceData = devices.find((d) => d.id === activeDevice);
  const isTriggered = activeDeviceData && activeDeviceData.status === "TRIGGERED";

  return (
    <div className={`app-container ${isTriggered ? "app-triggered" : ""}`}>
      <header className="app-header">
        <div className="brand">
          <span className="brand-logo">LapGuard</span>
          <span className="brand-tag">Dashboard</span>
        </div>
        <div style={{ display: "flex", alignItems: "center", gap: "16px" }}>
          <span style={{ fontSize: "14px", color: "var(--text-secondary)" }}>
            Hi, <strong>{user.email}</strong>
          </span>
          <button className="btn btn-secondary" onClick={handleLogout}>
            ĐĂNG XUẤT
          </button>
        </div>
      </header>

      <div className="dashboard-grid">
        <div style={{ display: "flex", flexDirection: "column", gap: "24px" }}>
          <Dashboard activeDevice={activeDevice} deviceData={activeDeviceData} />
          {activeDevice && <AlarmLogs activeDevice={activeDevice} logs={logs} />}
        </div>
        <div>
          <DeviceManager
            user={user}
            devices={devices}
            activeDevice={activeDevice}
            setActiveDevice={setActiveDevice}
          />
        </div>
      </div>
    </div>
  );
}
