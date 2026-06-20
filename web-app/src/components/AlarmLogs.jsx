import { ref, update } from "firebase/database";
import { db } from "../firebase";
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from "@/components/ui/card";
import { Table, TableBody, TableCell, TableHead, TableHeader, TableRow } from "@/components/ui/table";
import { Button } from "@/components/ui/button";
import { History, AlertTriangle, CheckCircle2, Battery, RefreshCw } from "lucide-react";
import { toast } from "sonner";

export default function AlarmLogs({ activeDevice, logs }) {
  const deviceLogs = logs
    .filter((log) => log.device_id === activeDevice)
    .sort((a, b) => b.timestamp - a.timestamp);

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
      toast.success("Đã đánh dấu đã giải quyết sự cố.");
    } catch (err) {
      console.error("Failed to resolve log:", err);
      toast.error("Không thể cập nhật trạng thái nhật ký.");
    }
  };

  return (
    <Card className="bg-background/60 backdrop-blur-md border-border/50 shadow-xl dynamic-card">
      <CardHeader>
        <CardTitle className="text-lg font-bold tracking-tight flex items-center gap-2">
          <History className="h-5 w-5 text-primary" /> Nhật ký hoạt động
        </CardTitle>
        <CardDescription>Lịch sử các sự kiện cảnh báo của thiết bị này</CardDescription>
      </CardHeader>
      <CardContent>
        {deviceLogs.length === 0 ? (
          <div className="text-center py-12 text-sm text-muted-foreground border border-dashed border-border/40 rounded-lg bg-black/10">
            Chưa có ghi chép nhật ký nào
          </div>
        ) : (
          <div className="overflow-hidden border border-border/40 rounded-lg">
            <Table>
              <TableHeader className="bg-black/10">
                <TableRow>
                  <TableHead className="text-xs uppercase tracking-wider text-muted-foreground font-semibold">Thời gian</TableHead>
                  <TableHead className="text-xs uppercase tracking-wider text-muted-foreground font-semibold">Sự kiện</TableHead>
                  <TableHead className="text-xs uppercase tracking-wider text-muted-foreground font-semibold">Chi tiết</TableHead>
                  <TableHead className="text-xs uppercase tracking-wider text-muted-foreground font-semibold">Trạng thái</TableHead>
                  <TableHead className="text-xs uppercase tracking-wider text-muted-foreground font-semibold text-right">Thao tác</TableHead>
                </TableRow>
              </TableHeader>
              <TableBody>
                {deviceLogs.map((log) => (
                  <TableRow key={log.id} className="hover:bg-black/5 border-b border-border/30">
                    <TableCell className="font-mono text-xs text-muted-foreground whitespace-nowrap">
                      {formatTimestamp(log.timestamp)}
                    </TableCell>
                    <TableCell>
                      <span className={`inline-flex items-center gap-1 px-2.5 py-0.5 rounded-full text-xs font-semibold border shadow-sm ${
                        log.event_type === "MOTION_ALERT"
                          ? "bg-red-500/10 text-red-500 border-red-500/20"
                          : log.event_type === "BATTERY_LOW"
                          ? "bg-amber-500/10 text-amber-500 border-amber-500/20"
                          : "bg-primary/10 text-primary border-primary/20"
                      }`}>
                        {log.event_type === "MOTION_ALERT" ? (
                          <AlertTriangle className="h-3 w-3" />
                        ) : log.event_type === "BATTERY_LOW" ? (
                          <Battery className="h-3 w-3" />
                        ) : (
                          <RefreshCw className="h-3 w-3" />
                        )}
                        {log.event_type === "MOTION_ALERT"
                          ? "Báo động"
                          : log.event_type === "BATTERY_LOW"
                          ? "Pin yếu"
                          : log.event_type}
                      </span>
                    </TableCell>
                    <TableCell className={`text-sm ${log.event_type === "MOTION_ALERT" ? "font-semibold" : ""}`}>
                      {log.detail}
                    </TableCell>
                    <TableCell>
                      {log.resolved ? (
                        <span className="inline-flex items-center gap-1 text-xs font-semibold text-green-500">
                          <CheckCircle2 className="h-3 w-3" /> Đã xử lý
                        </span>
                      ) : (
                        <span className="inline-flex items-center gap-1 text-xs font-semibold text-red-500">
                          <AlertTriangle className="h-3 w-3" /> Chưa xử lý
                        </span>
                      )}
                    </TableCell>
                    <TableCell className="text-right">
                      {!log.resolved && log.event_type === "MOTION_ALERT" ? (
                        <Button
                          variant="outline"
                          size="sm"
                          className="h-7 px-3 text-xs border-border/60 hover:bg-green-600 hover:text-white hover:border-green-600 rounded-lg dynamic-button"
                          onClick={() => handleResolve(log.id)}
                        >
                          Giải quyết
                        </Button>
                      ) : (
                        <span className="text-xs text-muted-foreground">—</span>
                      )}
                    </TableCell>
                  </TableRow>
                ))}
              </TableBody>
            </Table>
          </div>
        )}
      </CardContent>
    </Card>
  );
}
