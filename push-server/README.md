# LapGuard Push Server

This is a small Node.js server that sends FCM Web Push notifications without Firebase Cloud Functions.

It watches Firebase Realtime Database logs:

```text
/logs/{logId}
```

When a `MOTION_ALERT` or `BATTERY_LOW` log appears, it:

1. Reads `/devices/<device_id>/owner_id`.
2. Reads `/users/<uid>/fcm_tokens`.
3. Sends FCM push notifications to those browser tokens.

## Why This Exists

Firebase Cloud Functions requires the Blaze plan. This server can run locally or on any Node.js hosting provider instead.

## Setup

1. In Firebase Console, create a service account key:
   - Project settings
   - Service accounts
   - Generate new private key
2. Save it as:

```text
push-server/service-account.json
```

Do not commit that file.

3. Copy `.env.example` to `.env`:

```bash
cp .env.example .env
```

4. Fill:

```env
FIREBASE_DATABASE_URL=https://your-project-default-rtdb.asia-southeast1.firebasedatabase.app
GOOGLE_APPLICATION_CREDENTIALS=./service-account.json
PORT=3001
```

## Run

```bash
npm install
npm start
```

Health check:

```text
http://localhost:3001/health
```

## Notes

- The server only processes logs created after it starts.
- Keep this server running if you want push notifications while the Web App is closed.
- Web App open-tab notifications still work without this server because React listens to Realtime Database directly.
