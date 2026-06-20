import { useState } from "react";
import { ref, update } from "firebase/database";
import { db } from "../firebase";
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from "@/components/ui/card";
import { Input } from "@/components/ui/input";
import { Button } from "@/components/ui/button";
import { Laptop, Plus, Trash2, Key, Loader2, Link2 } from "lucide-react";
import { toast } from "sonner";

export default function DeviceManager({ user, devices, activeDevice, setActiveDevice }) {
  const [newMac, setNewMac] = useState("");
  const [newName, setNewName] = useState("");
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
    setLoading(true);

    const mac = formatMacAddress(newMac);
    if (!validateMac(mac)) {
      toast.error("Địa chỉ MAC không đúng định dạng. Ví dụ: 24:0A:C4:12:34:56");
      setLoading(false);
      return;
    }

    if (!newName.trim()) {
      toast.error("Vui lòng nhập tên thiết bị.");
      setLoading(false);
      return;
    }

    try {
      const updates = {};
      updates[`/devices/${mac}/owner_id`] = user.uid;
      updates[`/devices/${mac}/device_name`] = newName;
      
      const existingDevice = devices.find(d => d.id === mac);
      if (!existingDevice) {
        updates[`/devices/${mac}/status`] = "DISARMED";
        updates[`/devices/${mac}/command`] = "NONE";
        updates[`/devices/${mac}/battery_percent`] = 100;
        updates[`/devices/${mac}/wifi_rssi`] = 0;
      }
      
      updates[`/users/${user.uid}/devices/${mac}`] = true;

      await update(ref(db), updates);
      
      toast.success("Đã liên kết thiết bị thành công!");
      setNewMac("");
      setNewName("");
      setActiveDevice(mac);
    } catch (err) {
      console.error(err);
      toast.error("Không thể liên kết thiết bị. Vui lòng kiểm tra phân quyền.");
    } finally {
      setLoading(false);
    }
  };

  const handleRemoveDevice = async (mac, e) => {
    e.stopPropagation();
    if (!window.confirm(`Bạn có chắc chắn muốn hủy liên kết thiết bị ${mac}?`)) return;

    try {
      const updates = {};
      updates[`/users/${user.uid}/devices/${mac}`] = null;
      updates[`/devices/${mac}/owner_id`] = null;

      await update(ref(db), updates);
      toast.success("Đã hủy liên kết thiết bị thành công.");
      
      if (activeDevice === mac) {
        const remaining = devices.filter(d => d.id !== mac);
        setActiveDevice(remaining.length > 0 ? remaining[0].id : "");
      }
    } catch (err) {
      console.error(err);
      toast.error("Không thể hủy liên kết thiết bị.");
    }
  };

  return (
    <div className="space-y-6">
      <Card className="bg-background/60 backdrop-blur-md border-border/50 shadow-xl dynamic-card">
        <CardHeader>
          <CardTitle className="text-lg font-bold tracking-tight flex items-center gap-2">
            <Laptop className="h-5 w-5 text-primary" /> Danh sách thiết bị
          </CardTitle>
          <CardDescription>Chọn hoặc hủy liên kết các thiết bị LapGuard của bạn</CardDescription>
        </CardHeader>
        <CardContent className="space-y-4">
          {devices.length === 0 ? (
            <div className="text-center py-8 text-sm text-muted-foreground border border-dashed border-border/40 rounded-lg bg-black/10">
              Chưa có thiết bị nào được liên kết
            </div>
          ) : (
            <div className="space-y-2.5">
              {devices.map((dev) => (
                <div
                  key={dev.id}
                  className={`flex justify-between items-center p-3.5 rounded-xl border transition-all duration-300 cursor-pointer dynamic-item ${
                    activeDevice === dev.id
                      ? "border-primary bg-primary/5 shadow-sm"
                      : "border-border/40 bg-black/5 hover:bg-black/10 hover:border-border/70"
                  }`}
                  onClick={() => setActiveDevice(dev.id)}
                >
                  <div className="space-y-1">
                    <div className="font-semibold text-sm leading-none flex items-center gap-2">
                      <span className={`w-2 h-2 rounded-full ${
                        dev.status === "TRIGGERED" ? "bg-red-600 animate-pulse" : dev.status === "ARMED" ? "bg-red-500" : "bg-green-500"
                      }`} />
                      {dev.device_name || "Thiết bị không tên"}
                    </div>
                    <div className="font-mono text-xs text-muted-foreground">{dev.id}</div>
                  </div>
                  <Button
                    variant="ghost"
                    size="icon"
                    className="h-8 w-8 text-muted-foreground hover:text-red-500 hover:bg-red-500/10 rounded-lg"
                    onClick={(e) => handleRemoveDevice(dev.id, e)}
                  >
                    <Trash2 className="h-4 w-4" />
                  </Button>
                </div>
              ))}
            </div>
          )}
        </CardContent>
      </Card>

      <Card className="bg-background/60 backdrop-blur-md border-border/50 shadow-xl dynamic-card">
        <CardHeader>
          <CardTitle className="text-sm font-bold tracking-tight uppercase flex items-center gap-2 text-muted-foreground">
            <Link2 className="h-4 w-4" /> Liên kết thiết bị mới
          </CardTitle>
        </CardHeader>
        <form onSubmit={handleAddDevice}>
          <CardContent className="space-y-4">
            <div className="space-y-2">
              <label className="text-xs font-semibold text-muted-foreground uppercase tracking-wider flex items-center gap-1.5">
                <Key className="h-3.5 w-3.5" /> Địa chỉ MAC
              </label>
              <Input
                type="text"
                placeholder="24:0A:C4:12:34:56"
                value={newMac}
                onChange={(e) => setNewMac(e.target.value)}
                className="bg-black/20 border-border/50"
                required
              />
            </div>

            <div className="space-y-2">
              <label className="text-xs font-semibold text-muted-foreground uppercase tracking-wider flex items-center gap-1.5">
                <Laptop className="h-3.5 w-3.5" /> Tên gợi nhớ
              </label>
              <Input
                type="text"
                placeholder="Balo Laptop của An"
                value={newName}
                onChange={(e) => setNewName(e.target.value)}
                className="bg-black/20 border-border/50"
                required
              />
            </div>
          </CardContent>
          <CardContent className="pt-0">
            <Button
              type="submit"
              className="w-full bg-primary hover:bg-primary/90 text-primary-foreground font-semibold flex items-center justify-center gap-1.5 rounded-xl shadow-md dynamic-button"
              disabled={loading}
            >
              {loading ? (
                <>
                  <Loader2 className="h-4 w-4 animate-spin" /> Đang lưu...
                </>
              ) : (
                <>
                  <Plus className="h-4 w-4" /> LIÊN KẾT THIẾT BỊ
                </>
              )}
            </Button>
          </CardContent>
        </form>
      </Card>
    </div>
  );
}
