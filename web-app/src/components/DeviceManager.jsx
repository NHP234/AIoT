import React, { useState } from "react";
import { ref, update, set } from "firebase/database";
import { db } from "../firebase";

export default function DeviceManager({ user, devices, activeDevice, setActiveDevice }) {
  const [newMac, setNewMac] = useState("");
  const [newName, setNewName] = useState("");
  const [error, setError] = useState("");
  const [success, setSuccess] = useState("");
  const [loading, setLoading] = useState(false);

  const formatMacAddress = (mac) => {
    return mac.trim().toUpperCase().replace(/-/g, ":");
  };

  const validateMac = (mac) => {
    const regex = /^([0-9A-FA-F]{2}[:-]){5}([0-9A-FA-F]{2})$/;
    return regex.test(mac);
  };

  const handleAddDevice = async (e) => {
    e.preventDefault();
    setError("");
    setSuccess("");
    setLoading(true);

    const mac = formatMacAddress(newMac);
    if (!validateMac(mac)) {
      setError("Địa chỉ MAC không đúng định dạng. Ví dụ: 24:0A:C4:12:34:56");
      setLoading(false);
      return;
    }

    if (!newName.trim()) {
      setError("Vui lòng nhập tên thiết bị.");
      setLoading(false);
      return;
    }

    try {
      const updates = {};
      // 1. Map MAC to owner under /devices/$mac
      updates[`/devices/${mac}/owner_id`] = user.uid;
      updates[`/devices/${mac}/device_name`] = newName;
      // Initialize state fields only if they do not exist
      const existingDevice = devices.find(d => d.id === mac);
      if (!existingDevice) {
        updates[`/devices/${mac}/status`] = "DISARMED";
        updates[`/devices/${mac}/command`] = "NONE";
        updates[`/devices/${mac}/battery_percent`] = 100;
        updates[`/devices/${mac}/wifi_rssi`] = 0;
      }
      
      // 2. Add to user device inventory under /users/$uid/devices/$mac
      updates[`/users/${user.uid}/devices/${mac}`] = true;

      await update(ref(db), updates);
      
      setSuccess("Đã liên kết thiết bị thành công!");
      setNewMac("");
      setNewName("");
      setActiveDevice(mac);
    } catch (err) {
      console.error(err);
      setError("Không thể liên kết thiết bị. Vui lòng kiểm tra quyền truy cập.");
    } finally {
      setLoading(false);
    }
  };

  const handleRemoveDevice = async (mac, e) => {
    e.stopPropagation(); // Avoid selecting the device when clicking delete
    if (!window.confirm(`Bạn có chắc chắn muốn hủy liên kết thiết bị ${mac}?`)) return;

    setError("");
    setSuccess("");

    try {
      const updates = {};
      updates[`/users/${user.uid}/devices/${mac}`] = null;
      updates[`/devices/${mac}/owner_id`] = null; // Free device for someone else or mark orphaned

      await update(ref(db), updates);
      setSuccess("Đã hủy liên kết thiết bị.");
      
      if (activeDevice === mac) {
        // Switch to another device if available, else null
        const remaining = devices.filter(d => d.id !== mac);
        setActiveDevice(remaining.length > 0 ? remaining[0].id : "");
      }
    } catch (err) {
      console.error(err);
      setError("Không thể hủy liên kết thiết bị.");
    }
  };

  return (
    <div className="glass-card device-manager-card">
      <h3 className="widget-title">Thiết bị của bạn</h3>

      {devices.length === 0 ? (
        <div className="empty-state">
          <p>Bạn chưa liên kết thiết bị LapGuard nào.</p>
        </div>
      ) : (
        <div className="device-list">
          {devices.map((dev) => (
            <div
              key={dev.id}
              className={`device-item ${activeDevice === dev.id ? "active" : ""}`}
              onClick={() => setActiveDevice(dev.id)}
            >
              <div className="device-item-info">
                <span className="device-item-name">{dev.device_name || "Thiết bị không tên"}</span>
                <span className="device-item-mac">{dev.id}</span>
              </div>
              <button
                className="btn btn-danger"
                style={{ padding: "6px 12px", fontSize: "12px" }}
                onClick={(e) => handleRemoveDevice(dev.id, e)}
              >
                HỦY
              </button>
            </div>
          ))}
        </div>
      )}

      <form onSubmit={handleAddDevice} className="device-manager-form">
        <h4 style={{ fontSize: "15px", fontWeight: "600", textTransform: "uppercase", letterSpacing: "0.5px" }}>
          Liên kết thiết bị mới
        </h4>
        
        <div className="form-group" style={{ marginBottom: "12px" }}>
          <label className="form-label">Địa chỉ MAC</label>
          <input
            type="text"
            className="form-input"
            placeholder="24:0A:C4:12:34:56"
            value={newMac}
            onChange={(e) => setNewMac(e.target.value)}
            required
          />
        </div>

        <div className="form-group" style={{ marginBottom: "12px" }}>
          <label className="form-label">Tên gợi nhớ</label>
          <input
            type="text"
            className="form-input"
            placeholder="Balo Laptop, Vali cá nhân..."
            value={newName}
            onChange={(e) => setNewName(e.target.value)}
            required
          />
        </div>

        {error && <div className="error-message">{error}</div>}
        {success && <div className="success-message">{success}</div>}

        <div className="device-manager-actions">
          <button type="submit" className="btn btn-primary" disabled={loading}>
            {loading ? "Đang lưu..." : "LIÊN KẾT"}
          </button>
        </div>
      </form>
    </div>
  );
}
