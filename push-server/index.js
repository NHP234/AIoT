require("dotenv").config();

const express = require("express");
const admin = require("firebase-admin");

const databaseUrl = process.env.FIREBASE_DATABASE_URL;
const port = Number(process.env.PORT || 3001);
const startedAtMs = Date.now();

function loadCredential() {
  if (process.env.FIREBASE_SERVICE_ACCOUNT_JSON) {
    return admin.credential.cert(JSON.parse(process.env.FIREBASE_SERVICE_ACCOUNT_JSON));
  }

  return admin.credential.applicationDefault();
}

function initFirebase() {
  if (!databaseUrl) {
    throw new Error("FIREBASE_DATABASE_URL is required.");
  }

  admin.initializeApp({
    credential: loadCredential(),
    databaseURL: databaseUrl,
  });
}

function notificationForLog(log, device) {
  const deviceName = device.device_name || log.device_id || "LapGuard";

  if (log.event_type === "MOTION_ALERT") {
    return {
      title: "CẢNH BÁO LAPGUARD",
      body: `${deviceName}: phát hiện di chuyển bất thường.`,
    };
  }

  if (log.event_type === "BATTERY_LOW") {
    return {
      title: "LapGuard pin yếu",
      body: `${deviceName}: ${log.detail || "vui lòng sạc lại thiết bị."}`,
    };
  }

  return {
    title: "LapGuard",
    body: log.detail || `${deviceName}: có sự kiện mới.`,
  };
}

function tokenEntriesToTokens(tokenEntries) {
  if (!tokenEntries) {
    return [];
  }

  return Object.values(tokenEntries)
    .map((entry) => entry && entry.token)
    .filter(Boolean);
}

async function removeInvalidTokens(uid, tokenEntries, responses) {
  const updates = {};
  const keys = Object.keys(tokenEntries || {});

  responses.forEach((response, index) => {
    if (response.success) {
      return;
    }

    const code = response.error && response.error.code;
    if (
      code === "messaging/registration-token-not-registered" ||
      code === "messaging/invalid-registration-token"
    ) {
      updates[`/users/${uid}/fcm_tokens/${keys[index]}`] = null;
    }
  });

  if (Object.keys(updates).length > 0) {
    await admin.database().ref().update(updates);
  }
}

async function sendPushForLog(logId, log) {
  if (!log || !log.device_id) {
    console.warn("[PUSH] Log missing device_id, skipping", { logId });
    return;
  }

  if (log.push_sent_at) {
    return;
  }

  if (log.event_type !== "MOTION_ALERT" && log.event_type !== "BATTERY_LOW") {
    return;
  }

  const database = admin.database();
  const deviceSnapshot = await database.ref(`/devices/${log.device_id}`).get();
  const device = deviceSnapshot.val() || {};
  const ownerId = device.owner_id;

  if (!ownerId) {
    console.warn("[PUSH] Device has no owner_id, skipping", { deviceId: log.device_id });
    return;
  }

  const tokenSnapshot = await database.ref(`/users/${ownerId}/fcm_tokens`).get();
  const tokenEntries = tokenSnapshot.val() || {};
  const tokens = tokenEntriesToTokens(tokenEntries);

  if (tokens.length === 0) {
    console.info("[PUSH] User has no FCM tokens, skipping", { ownerId });
    return;
  }

  const notification = notificationForLog(log, device);
  const response = await admin.messaging().sendEachForMulticast({
    tokens,
    notification,
    data: {
      logId,
      deviceId: log.device_id,
      eventType: log.event_type,
      title: notification.title,
      body: notification.body,
      tag: `lapguard-${log.device_id}`,
      url: "/",
      requireInteraction: "true",
    },
    webpush: {
      notification: {
        icon: "/favicon.svg",
        badge: "/favicon.svg",
        tag: `lapguard-${log.device_id}`,
        requireInteraction: true,
      },
    },
  });

  await removeInvalidTokens(ownerId, tokenEntries, response.responses);

  if (response.successCount > 0) {
    await database.ref(`/logs/${logId}`).update({
      push_sent_at: admin.database.ServerValue.TIMESTAMP,
      push_success_count: response.successCount,
      push_failure_count: response.failureCount,
    });
  }

  console.info("[PUSH] FCM push sent", {
    logId,
    successCount: response.successCount,
    failureCount: response.failureCount,
  });
}

function startLogWatcher() {
  const query = admin
    .database()
    .ref("/logs")
    .orderByChild("timestamp")
    .startAt(startedAtMs);

  query.on(
    "child_added",
    (snapshot) => {
      sendPushForLog(snapshot.key, snapshot.val()).catch((err) => {
        console.error("[PUSH] Failed to send push", {
          logId: snapshot.key,
          message: err.message,
        });
      });
    },
    (err) => {
      console.error("[PUSH] Log watcher failed", err);
    }
  );

  console.info("[PUSH] Watching /logs for new alerts", { startedAtMs });
}

function startHttpServer() {
  const app = express();
  app.use(express.json());

  app.get("/health", (_req, res) => {
    res.json({
      ok: true,
      startedAtMs,
      databaseUrl,
    });
  });

  app.listen(port, () => {
    console.info(`[PUSH] Server listening on :${port}`);
  });
}

initFirebase();
startLogWatcher();
startHttpServer();
