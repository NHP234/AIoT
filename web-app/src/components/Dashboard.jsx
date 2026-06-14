import React from "react";
import { ref, update } from "firebase/database";
import { db } from "../firebase";
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from "@/components/ui/card";
import { Button } from "@/components/ui/button";
import { Shield, ShieldAlert, VolumeX, Battery, Wifi, Clock, AlertTriangle } from "lucide-react";
import { toast } from "sonner";

export default function Dashboard({ activeDevice, deviceData }) {
  if (!activeDevice || !deviceData) {
    return (
      <Card className="bg-background/60 backdrop-blur-md border-border/50 shadow-xl min-h-[350px] flex items-center justify-center">
        <CardContent className="text-center py-8 text-muted-foreground space-y-2">
          <p className="font-semibold">Không có thiết bị hoạt động nào được chọn.</p>
          <p className="text-xs">Vui lòng chọn hoặc liên kết thiết bị ở danh sách bên cạnh.</p>
        </CardContent>
      </Card>
    );
  }

  const { status, battery_percent = 0, wifi_rssi = 0, last_seen = 0, device_name } = deviceData;

  const sendCommand = async (cmd) => {
    try {
      await update(ref(db, `devices/${activeDevice}`), { command: cmd });
      if (cmd === "ARM") {
        toast.info("Đã gửi lệnh KÍCH HOẠT (ARM) đến thiết bị.");
      } else if (cmd === "DISARM") {
        toast.info("Đã gửi lệnh TẮT BẢO VỆ (DISARM) đến thiết bị.");
      } else if (cmd === "SILENCE") {
        toast.info("Đã gửi lệnh TẮT CÒI BÁO ĐỘNG (SILENCE) đến thiết bị.");
      }
    } catch (err) {
      console.error("Failed to send command:", err);
      toast.error("Gửi lệnh thất bại. Vui lòng kiểm tra kết nối.");
    }
  };

  const getBatteryColor = (percent) => {
    if (percent > 50) return "bg-green-500";
    if (percent > 20) return "bg-amber-500";
    return "bg-red-500";
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

  const isDeviceOffline = () => {
    if (!last_seen) return true;
    const now = Date.now();
    return (now - last_seen) > 30000;
  };

  const currentStatus = isDeviceOffline() ? "OFFLINE" : status;

  return (
    <Card className={`bg-background/60 backdrop-blur-md border-border/50 shadow-xl overflow-hidden ${
      currentStatus === "TRIGGERED" ? "app-triggered" : ""
    }`}>
      <CardHeader className="border-b border-border/30 pb-4">
        <div className="flex flex-col sm:flex-row sm:justify-between sm:items-center gap-2">
          <CardTitle className="text-xl font-bold tracking-tight">
            Giám sát: {device_name || "Thiết bị LapGuard"}
          </CardTitle>
          <span className="font-mono text-xs text-muted-foreground bg-black/20 px-3 py-1 rounded-full border border-border/30 w-fit">
            {activeDevice}
          </span>
        </div>
      </CardHeader>
      
      <CardContent className="flex flex-col items-center justify-center py-10 text-center">
        {/* Status circle visualizer */}
        <div className="relative w-[190px] height-[190px] flex items-center justify-center mb-8">
          {/* Breathing outer glow */}
          <div className={`absolute w-[180px] h-[180px] rounded-full border-4 transition-all duration-1000 ${
            currentStatus === "DISARMED" 
              ? "border-green-500 shadow-[0_0_20px_rgba(34,197,94,0.2)]"
              : currentStatus === "ARMED" 
              ? "border-red-500 animate-pulse" 
              : currentStatus === "TRIGGERED" 
              ? "border-red-600 animate-ping opacity-75"
              : "border-muted/50 shadow-inner"
          }`} />
          
          <div className="z-10 flex flex-col items-center justify-center bg-black/25 dark:bg-black/40 border border-border/40 w-[150px] h-[150px] rounded-full backdrop-blur-md shadow-2xl">
            {currentStatus === "TRIGGERED" ? (
              <ShieldAlert className="h-9 w-9 text-red-500 animate-bounce mb-1" />
            ) : (
              <Shield className={`h-9 w-9 mb-1 ${
                currentStatus === "ARMED" 
                  ? "text-red-400 animate-pulse" 
                  : currentStatus === "DISARMED" 
                  ? "text-green-400" 
                  : "text-muted-foreground"
              }`} />
            )}
            <span className={`text-xl font-black tracking-wide uppercase ${
              currentStatus === "DISARMED"
                ? "text-green-400"
                : currentStatus === "ARMED"
                ? "text-red-400"
                : currentStatus === "TRIGGERED"
                ? "text-red-500 font-extrabold animate-pulse"
                : "text-muted-foreground"
            }`}>
              {currentStatus === "DISARMED"
                ? "AN TOÀN"
                : currentStatus === "ARMED"
                ? "BẢO VỆ"
                : currentStatus === "TRIGGERED"
                ? "CẢNH BÁO"
                : "NGOẠI TUYẾN"}
            </span>
            <span className="text-[10px] text-muted-foreground uppercase font-bold tracking-wider mt-0.5">Trạng thái</span>
          </div>
        </div>

        {/* Buttons Controls */}
        <div className="grid grid-cols-2 gap-4 w-full max-w-[420px]">
          <Button
            size="lg"
            className="bg-red-500 hover:bg-red-600 text-black font-semibold flex items-center justify-center gap-1.5 rounded-xl shadow-lg shadow-red-500/10"
            onClick={() => sendCommand("ARM")}
            disabled={currentStatus === "OFFLINE"}
          >
            KÍCH HOẠT (ARM)
          </Button>
          <Button
            size="lg"
            variant="outline"
            className="border-border/60 hover:bg-black/10 text-foreground font-semibold flex items-center justify-center gap-1.5 rounded-xl"
            onClick={() => sendCommand("DISARM")}
            disabled={currentStatus === "OFFLINE"}
          >
            TẮT BẢO VỆ
          </Button>
        </div>

        {currentStatus === "TRIGGERED" && (
          <Button
            variant="destructive"
            size="lg"
            className="w-full max-w-[420px] mt-4 font-bold flex items-center justify-center gap-1.5 rounded-xl animate-bounce"
            onClick={() => sendCommand("SILENCE")}
          >
            <VolumeX className="h-5 w-5" /> TẮT CÒI BÁO ĐỘNG (SILENCE)
          </Button>
        )}

        {/* Information Grid Widgets */}
        <div className="w-full max-w-[420px] mt-10 space-y-3">
          <div className="flex justify-between items-center p-3.5 rounded-xl border border-border/30 bg-black/10 dark:bg-black/20 text-sm">
            <span className="text-muted-foreground font-medium flex items-center gap-2">
              <Battery className="h-4 w-4 text-cyan-400" /> Dung lượng pin
            </span>
            <div className="flex items-center gap-2.5">
              <span className="font-semibold">{battery_percent}%</span>
              <div className="w-11 h-5.5 border-2 border-muted-foreground/60 rounded p-0.5 relative after:content-[''] after:absolute after:top-1.5 after:right-[-4px] after:w-1 after:height-1.5 after:bg-muted-foreground/60 after:rounded-r">
                <div
                  className={`h-full rounded-sm transition-all duration-300 ${getBatteryColor(battery_percent)}`}
                  style={{ width: `${battery_percent}%` }}
                />
              </div>
            </div>
          </div>

          <div className="flex justify-between items-center p-3.5 rounded-xl border border-border/30 bg-black/10 dark:bg-black/20 text-sm">
            <span className="text-muted-foreground font-medium flex items-center gap-2">
              <Wifi className="h-4 w-4 text-cyan-400" /> Sóng WiFi
            </span>
            <div className="flex items-center gap-2.5">
              <span className="font-semibold">{wifi_rssi} dBm</span>
              <div className="flex items-end gap-[2px] h-3.5">
                {[1, 2, 3, 4].map((bar) => (
                  <div
                    key={bar}
                    className={`w-[3px] rounded-sm transition-all duration-300 ${
                      bar <= signalBars ? "bg-cyan-400" : "bg-muted"
                    }`}
                    style={{ height: `${bar * 3.5}px` }}
                  />
                ))}
              </div>
            </div>
          </div>

          <div className="flex justify-between items-center p-3.5 rounded-xl border border-border/30 bg-black/10 dark:bg-black/20 text-sm">
            <span className="text-muted-foreground font-medium flex items-center gap-2">
              <Clock className="h-4 w-4 text-cyan-400" /> Kết nối cuối cùng
            </span>
            <span className="font-semibold">{formatLastSeen(last_seen)}</span>
          </div>
        </div>
      </CardContent>
    </Card>
  );
}
