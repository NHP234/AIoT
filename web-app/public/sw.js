const CACHE_NAME = "lapguard-cache-v1";
const ASSETS = [
  "/",
  "/index.html",
  "/favicon.svg",
  "/src/main.jsx",
  "/src/App.jsx",
  "/src/index.css",
  "/src/firebase.js",
  "/src/components/Auth.jsx",
  "/src/components/Dashboard.jsx",
  "/src/components/DeviceManager.jsx",
  "/src/components/AlarmLogs.jsx"
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
