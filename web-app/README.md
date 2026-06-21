# LapGuard Web App

React + Vite dashboard for LapGuard. The app uses Firebase Auth, Realtime Database, and Firebase Cloud Messaging (FCM) browser tokens for Web Push.

## Setup

1. Copy `.env.example` to `.env`.
2. Fill Firebase Web App config values.
3. Generate a Web Push certificate in Firebase Console:
   - Project settings
   - Cloud Messaging
   - Web Push certificates
   - Generate key pair
4. Put the public key into:

```env
VITE_FIREBASE_VAPID_KEY=...
```

## Commands

```bash
npm install
npm run dev
npm run lint
npm run build
```

## FCM Flow

When a user logs in, the app:

1. Registers `/sw.js`.
2. Requests notification permission.
3. Gets an FCM registration token.
4. Stores the token at `/users/<uid>/fcm_tokens/<token_key>`.

When the user logs out, the app removes the current browser token from that path so the signed-out browser stops receiving pushes for that account.

The Node server in `../push-server` watches `/logs/{logId}` and sends push notifications to the device owner's tokens.

Web Push requires a secure context. It works on `localhost` during development and on HTTPS in production.

Firebase Hosting can be deployed on the free Spark plan:

```bash
npm run build
cd ..
npx firebase-tools deploy --only hosting --project aiot-929e6
```
