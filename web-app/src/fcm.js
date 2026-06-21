import { deleteToken, getToken, onMessage } from "firebase/messaging";
import { ref, remove, serverTimestamp, set } from "firebase/database";

import { db, getMessagingIfSupported } from "./firebase";
import { getServiceWorkerRegistration } from "./registerServiceWorker";

const vapidKey = import.meta.env.VITE_FIREBASE_VAPID_KEY;

function tokenToKey(token) {
  return token.replace(/[.#$/[\]]/g, "_");
}

function browserName() {
  return navigator.userAgent || "unknown";
}

export async function setupFcm(user, handlers = {}) {
  if (!user || !("Notification" in window)) {
    return () => {};
  }

  if (!vapidKey) {
    console.info("[FCM] VITE_FIREBASE_VAPID_KEY is not configured; push notifications disabled.");
    return () => {};
  }

  const messaging = await getMessagingIfSupported();
  if (!messaging) {
    console.info("[FCM] Firebase Messaging is not supported in this browser.");
    return () => {};
  }

  const permission = await Notification.requestPermission();
  if (permission !== "granted") {
    console.info("[FCM] Notification permission not granted.");
    return () => {};
  }

  const serviceWorkerRegistration = await getServiceWorkerRegistration();
  if (!serviceWorkerRegistration) {
    console.info("[FCM] Service worker is not available; push notifications disabled.");
    return () => {};
  }

  const token = await getToken(messaging, {
    vapidKey,
    serviceWorkerRegistration,
  });

  if (!token) {
    console.info("[FCM] No registration token returned.");
    return () => {};
  }

  await set(ref(db, `users/${user.uid}/fcm_tokens/${tokenToKey(token)}`), {
    token,
    user_agent: browserName(),
    updated_at: serverTimestamp(),
  });

  console.info("[FCM] Registration token stored.");

  return onMessage(messaging, (payload) => {
    handlers.onForegroundMessage?.(payload);
  });
}

export async function unregisterFcmToken(user) {
  if (!user || !vapidKey || !("Notification" in window)) {
    return;
  }

  const messaging = await getMessagingIfSupported();
  if (!messaging) {
    return;
  }

  const serviceWorkerRegistration = await getServiceWorkerRegistration();
  if (!serviceWorkerRegistration) {
    return;
  }

  const token = await getToken(messaging, {
    vapidKey,
    serviceWorkerRegistration,
  });

  if (!token) {
    return;
  }

  await remove(ref(db, `users/${user.uid}/fcm_tokens/${tokenToKey(token)}`));

  try {
    await deleteToken(messaging);
  } catch (err) {
    console.info("[FCM] Token removed from database, but browser token deletion failed:", err);
  }
}
