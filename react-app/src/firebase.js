// ============================================================================
//  firebase.js  —  אתחול Firebase לאפליקציה (Auth + Cloud Firestore)
//  ----------------------------------------------------------------------------
//  הפרויקט: autobraid-5b08e (זהה לפרויקט שכבר מוגדר ב-esp32_firebase/Config.h)
//
//  מבנה הנתונים המשותף (Firestore collections):
//    users/{uid}   = { name, email, role, createdAt }
//    codes/{code}  = { uid, name, used:false, createdAt }
//    orders/{id}   = { uid, name, extensions, hairColor, createdAt }
// ============================================================================
import { initializeApp } from "firebase/app";
import { getAuth } from "firebase/auth";
import { getFirestore } from "firebase/firestore";

const firebaseConfig = {
  apiKey:     "AIzaSyC109K7mwhpLZr2lqKCewPHYNQrZagIuAY",
  authDomain: "autobraid-5b08e.firebaseapp.com",
  projectId:  "autobraid-5b08e",
  storageBucket: "autobraid-5b08e.appspot.com",
};

const app = initializeApp(firebaseConfig);
export const auth = getAuth(app);
export const db   = getFirestore(app);
