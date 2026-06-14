import React from "react";
import { ref, update } from "firebase/database";
import { db } from "../firebase";

export default function AlarmLogs({ activeDevice, logs }) {
  // Filter logs for the active device
  const deviceLogs = logs
    .filter((log) => log.device_id === activeDevice)
    .sort((a, b) => b.timestamp - a.timestamp); // Sort newest first

  const formatTimestamp = (ts) => {
    if (!ts) return "N/A";
    const date = new Date(ts);
    return date.toLocaleString("vi-VN", {
      hour: "2-digit",
      minute: "2-digit",
      second: "2-digit",
      day: "2-digit",
      month: "2-digit",
      year: "numeric",
    });
  };

  const handleResolve = async (logId) => {
    try {
      await update(ref(db, `logs/${logId}`), { resolved: true });
    } catch (err) {
      console.error("Failed to resolve log:", err);
    }
  };

  return (
    <div className="glass-card logs-card">
      <h3 className="widget-title">Nhật ký hoạt động</h3>

      {deviceLogs.length === 0 ? (
        <div className="empty-state">
          <p>Không có nhật ký nào cho thiết bị này.</p>
        </div>
      ) : (
        <div className="logs-table-wrapper">
          <table className="logs-table">
            <thead>
              <tr>
                <th>Thời gian</th>
                <th>Sự kiện</th>
                <th>Chi tiết</th>
                <th>Trạng thái</th>
                <th>Thao tác</th>
              </tr>
            </thead>
            <tbody>
              {deviceLogs.map((log) => (
                <tr key={log.id}>
                  <td className="log-timestamp">{formatTimestamp(log.timestamp)}</td>
                  <td>
                    <span className={`log-type-tag ${log.event_type}`}>
                      {log.event_type === "MOTION_ALERT" 
                        ? "BÁO ĐỘNG" 
                        : log.event_type === "BATTERY_LOW" 
                        ? "PIN YẾU" 
                        : log.event_type}
                    </span>
                  </td>
                  <td style={{ fontWeight: log.event_type === "MOTION_ALERT" ? "600" : "normal" }}>
                    {log.detail}
                  </td>
                  <td>
                    {log.resolved ? (
                      <span className="log-resolved">Đã xử lý</span>
                    ) : (
                      <span className="log-unresolved">Chưa xử lý</span>
                    )}
                  </td>
                  <td>
                    {!log.resolved && log.event_type === "MOTION_ALERT" ? (
                      <button
                        className="btn btn-secondary"
                        style={{ padding: "4px 10px", fontSize: "12px", borderRadius: "6px" }}
                        onClick={() => handleResolve(log.id)}
                      >
                        Giải quyết
                      </button>
                    ) : (
                      <span style={{ color: "var(--text-muted)", fontSize: "12px" }}>—</span>
                    )}
                  </td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      )}
    </div>
  );
}
