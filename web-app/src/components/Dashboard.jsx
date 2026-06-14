import React from "react";
import { ref, update } from "firebase/database";
import { db } from "../firebase";

export default function Dashboard({ activeDevice, deviceData }) {
  if (!activeDevice || !deviceData) {
    return (
      <div className="glass-card status-panel" style={{ minHeight: "350px" }}>
        <div className="empty-state">
          <p>Không có thiết bị hoạt động nào được chọn.</p>
          <p style={{ fontSize: "14px" }}>Vui lòng chọn hoặc liên kết thiết bị ở danh sách bên cạnh.</p>
        </div>
      </div>
    );
  }

  const { status, battery_percent = 0, wifi_rssi = 0, last_seen = 0, device_name } = deviceData;

  const sendCommand = async (cmd) => {
    try {
      await update(ref(db, `devices/${activeDevice}`), { command: cmd });
      console.log(`Command sent: ${cmd}`);
    } catch (err) {
      console.error("Failed to send command:", err);
      alert("Gửi lệnh thất bại. Vui lòng kiểm tra kết nối.");
    }
  };

  const getBatteryColor = (percent) => {
    if (percent > 50) return "#00e676"; // green
    if (percent > 20) return "#ff9800"; // orange
    return "#ff3366"; // red
  };

  const getSignalBarsCount = (rssi) => {
    if (rssi === 0 || rssi < -95) return 0;
    if (rssi > -60) return 4;
    if (rssi > -70) return 3;
    if (rssi > -80) return 2;
    return 1;
  };

  const formatLastSeen = (timestamp) => {
    if (!timestamp) return "Không rõ";
    const date = new Date(timestamp);
    return date.toLocaleTimeString("vi-VN") + " - " + date.toLocaleDateString("vi-VN");
  };

  const signalBars = getSignalBarsCount(wifi_rssi);

  // Check if device is offline (last seen > 30 seconds ago)
  const isDeviceOffline = () => {
    if (!last_seen) return true;
    const now = Date.now();
    return (now - last_seen) > 30000; // 30 seconds threshold
  };

  const currentStatus = isDeviceOffline() ? "OFFLINE" : status;

  return (
    <div className={`glass-card status-panel ${currentStatus === "TRIGGERED" ? "app-triggered" : ""}`}>
      <h3 className="widget-title" style={{ width: "100%", textAlign: "left", marginBottom: "30px" }}>
        <span>Giám sát: {device_name || "LapGuard Device"}</span>
        <span style={{ fontSize: "12px", fontFamily: "var(--font-mono)", color: "var(--text-secondary)" }}>
          {activeDevice}
        </span>
      </h3>

      <div className="status-ring-container">
        <div className={`status-ring ${currentStatus}`} />
        <div style={{ zIndex: 2 }}>
          <div className={`status-value ${currentStatus}`}>
            {currentStatus === "DISARMED" 
              ? "AN TOÀN" 
              : currentStatus === "ARMED" 
              ? "BẢO VỆ" 
              : currentStatus === "TRIGGERED" 
              ? "CẢNH BÁO" 
              : "NGOẠI TUYẾN"}
          </div>
          <div className="status-label">Trạng thái</div>
        </div>
      </div>

      <div className="control-actions">
        <button
          className="btn btn-primary btn-ctrl"
          onClick={() => sendCommand("ARM")}
          disabled={currentStatus === "OFFLINE"}
        >
          KÍCH HOẠT (ARM)
        </button>
        <button
          className="btn btn-secondary btn-ctrl"
          onClick={() => sendCommand("DISARM")}
          disabled={currentStatus === "OFFLINE"}
        >
          TẮT BẢO VỆ
        </button>
      </div>

      {currentStatus === "TRIGGERED" && (
        <button
          className="btn btn-danger btn-ctrl"
          style={{ width: "100%", marginTop: "12px" }}
          onClick={() => sendCommand("SILENCE")}
        >
          TẮT CÒI HÚ (SILENCE)
        </button>
      )}

      <div className="info-widgets" style={{ width: "100%", marginTop: "32px" }}>
        <div className="info-item">
          <span className="info-item-label">Mức Pin</span>
          <div className="info-item-value battery-container">
            <span>{battery_percent}%</span>
            <div className="battery-visual">
              <div
                className="battery-level"
                style={{
                  width: `${battery_percent}%`,
                  backgroundColor: getBatteryColor(battery_percent),
                }}
              />
            </div>
          </div>
        </div>

        <div className="info-item">
          <span className="info-item-label">Sóng WiFi</span>
          <div className="info-item-value" style={{ display: "flex", alignItems: "center", gap: "10px" }}>
            <span>{wifi_rssi} dBm</span>
            <div className="signal-container">
              {[1, 2, 3, 4].map((bar) => (
                <div
                  key={bar}
                  className={`signal-bar ${bar <= signalBars ? "active" : ""}`}
                  style={{ height: `${bar * 4}px` }}
                />
              ))}
            </div>
          </div>
        </div>

        <div className="info-item">
          <span className="info-item-label">Kết nối cuối</span>
          <span className="info-item-value">{formatLastSeen(last_seen)}</span>
        </div>
      </div>
    </div>
  );
}
