const CACHE_NAME = "lapguard-cache-v1";
const URGENT_VIBRATION_PATTERN = [400, 120, 400, 120, 900];
const ASSETS = [
  "/",
  "/index.html",
  "/favicon.svg",
  "/manifest.json"
];

// Install Event
self.addEventListener("install", (e) => {
  e.waitUntil(
    caches.open(CACHE_NAME).then((cache) => {
      console.log("[Service Worker] Caching files");
      return cache.addAll(ASSETS).catch(err => console.log("Caching error: ", err));
    })
  );
  self.skipWaiting();
});

// Activate Event
self.addEventListener("activate", (e) => {
  e.waitUntil(
    caches.keys().then((keys) => {
      return Promise.all(
        keys.map((key) => {
          if (key !== CACHE_NAME) {
            console.log("[Service Worker] Clearing old cache");
            return caches.delete(key);
          }
        })
      );
    })
  );
  self.clients.claim();
});

// Fetch Event
self.addEventListener("fetch", (e) => {
  // Only intercept HTTP/HTTPS schemes to avoid chrome-extension:// error logs
  if (!e.request.url.startsWith(self.location.origin)) {
    return;
  }
  
  e.respondWith(
    caches.match(e.request).then((cachedResponse) => {
      if (cachedResponse) {
        return cachedResponse;
      }
      return fetch(e.request).then((response) => {
        // Cache new successful requests dynamically
        if (response.status === 200) {
          const responseClone = response.clone();
          caches.open(CACHE_NAME).then((cache) => {
            cache.put(e.request, responseClone);
          });
        }
        return response;
      });
    }).catch(() => {
      // Fallback for offline API/HTML
      return caches.match("/");
    })
  );
});

self.addEventListener("push", (event) => {
  let payload;

  try {
    payload = event.data ? event.data.json() : {};
  } catch {
    payload = {
      notification: {
        title: "LapGuard",
        body: event.data ? event.data.text() : "New alert",
      },
    };
  }

  const notification = payload.notification || {};
  const data = payload.data || {};
  const vibrate = data.vibrate ? JSON.parse(data.vibrate) : notification.vibrate;
  const title = notification.title || data.title || "LapGuard alert";
  const options = {
    body: notification.body || data.body || "Thiết bị LapGuard có cảnh báo mới.",
    icon: notification.icon || "/favicon.svg",
    badge: "/favicon.svg",
    tag: data.tag || notification.tag || "lapguard-alert",
    requireInteraction: data.requireInteraction !== "false",
    renotify: data.renotify !== "false",
    silent: data.silent === "true",
    vibrate: vibrate || URGENT_VIBRATION_PATTERN,
    data: {
      url: data.url || "/",
      ...data,
    },
  };

  event.waitUntil(self.registration.showNotification(title, options));
});

self.addEventListener("notificationclick", (event) => {
  event.notification.close();
  const targetUrl = event.notification.data?.url || "/";

  event.waitUntil(
    self.clients.matchAll({ type: "window", includeUncontrolled: true }).then((clientList) => {
      for (const client of clientList) {
        if ("focus" in client) {
          client.navigate(targetUrl);
          return client.focus();
        }
      }

      if (self.clients.openWindow) {
        return self.clients.openWindow(targetUrl);
      }

      return undefined;
    })
  );
});
