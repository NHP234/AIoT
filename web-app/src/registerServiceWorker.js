let registrationPromise = null;

function canRegisterServiceWorker() {
  return (
    "serviceWorker" in navigator &&
    (import.meta.env.PROD ||
      window.location.hostname === "localhost" ||
      window.location.hostname === "127.0.0.1")
  );
}

export function register() {
  if (!canRegisterServiceWorker()) {
    return Promise.resolve(null);
  }

  if (!registrationPromise) {
    registrationPromise = navigator.serviceWorker
      .register("/sw.js")
      .then((reg) => {
        console.log("[Service Worker] Registered successfully: ", reg.scope);
        return reg;
      })
      .catch((err) => {
        registrationPromise = null;
        console.error("[Service Worker] Registration failed: ", err);
        return null;
      });
  }

  return registrationPromise;
}

export function getServiceWorkerRegistration() {
  return register();
}

export function unregister() {
  if ("serviceWorker" in navigator) {
    navigator.serviceWorker.ready
      .then((registration) => {
        registration.unregister();
      })
      .catch((error) => {
        console.error(error.message);
      });
  }
}
