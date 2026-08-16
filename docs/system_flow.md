# System Flow

End-to-end walkthrough of what happens when the machine is used, from the user's first tap in the
app to the order landing in Firestore. Cross-references the detailed docs for each subsystem rather
than repeating them.

## High-level flow

```text
User (app)
  ↓  register / log in, tap "Code for machine"
React App  →  HTTP  →  second_esp32 (Master) HTTP server  →  Firebase (Auth + Firestore)
  ↓  4-digit code shown in the app

User (machine)
  ↓
Touchscreen (second_esp32)
  ↓  code entry → validated against Firebase
  ↓  extension selection, insert hair, START
second_esp32 state machine
  ↓  UART: DIST? / COLOR? / DISP:<n>
first_esp32 (Slave)
  ↓  ultrasonic + color sensors, extension carousel motor
  ↓  UART replies
second_esp32 state machine
  ↓  ARMED → braid + rail motors run
  ↓  (button press mid-braid → EMG → emergency stop, see safety.md)
Mechanical action (braiding)
  ↓  rail returns home
second_esp32 → Firebase (order saved: completed or emergency)
  ↓
React App  →  HTTP  →  second_esp32 HTTP server  →  Firebase
  ↓  order shown in "My Appointments" / admin order history
```

## Sequence diagram

```mermaid
sequenceDiagram
    actor U as User
    participant App as React App
    participant M as second_esp32 (Master)
    participant F as first_esp32 (Slave)
    participant FB as Firebase

    U->>App: Register / Login
    App->>M: POST /register or /login
    M->>FB: Auth REST API
    FB-->>M: uid / name / role
    M-->>App: {uid, name, role}

    U->>App: Tap "Code for machine"
    App->>M: POST /generate-code
    M->>FB: create codes/{code}
    M-->>App: {code}

    U->>M: Enter code on touchscreen
    M->>FB: validate codes/{code}
    FB-->>M: name (or rejected)
    M-->>U: "Hi <name>" / "Invalid code"

    U->>M: Select up to 3 extensions, insert hair, START

    M->>F: DIST?
    F-->>M: DIST / DISTOK

    opt "MyHair" selected
        M->>F: COLOR?
        F-->>M: COLOR:<name>
    end

    loop each selected extension
        M->>F: DISP:<n>
        F-->>M: DISPDONE
        U->>M: take extension, CONTINUE
    end
    M->>F: DISP:0 (home)
    F-->>M: DISPDONE

    U->>M: START (ready to braid)
    M->>F: ARMED
    Note over M: braid + rail motors run (see mechanical_design.md)

    alt normal finish
        Note over M: 60s reached
        M->>F: DISARM
        U->>M: FINISH
    else emergency
        U->>F: press button (armed)
        F-->>M: EMG
        Note over M: motors stopped immediately
        M->>F: DISARM
        U->>F: press button (clear)
        F-->>M: OK
    end

    Note over M: rail returns home
    M->>FB: save orders/{id} (completed or emergency)

    U->>App: View order history
    App->>M: GET /my-orders or /all-orders
    M->>FB: list orders
    FB-->>M: orders[]
    M-->>App: orders[] (JSON)
```

## Web/Firebase data flow

The React app never touches Firebase directly — every read/write is proxied through the master
ESP32's HTTP API, which in turn calls Firebase's REST APIs.

```mermaid
flowchart LR
    APP["React App<br/>src/api.js"] -->|"HTTP :80<br/>/register /login /generate-code<br/>/my-orders /all-orders /delete-order"| WS["second_esp32<br/>WebServer.ino"]
    WS --> AUTH["AuthManager.ino<br/>(register/login/generate-code)"]
    WS --> FBM["FirebaseManager.ino<br/>(orders, code validation)"]
    AUTH -->|HTTPS REST| FBAUTH["Firebase Auth"]
    AUTH -->|HTTPS| FS1["Firestore: users/{uid}, codes/{code}"]
    FBM -->|HTTPS| FS2["Firestore: codes/{code}, orders/{id}"]
    MACHINE["second_esp32<br/>state machine"] -->|"fbValidateCode / fbSaveOrder /<br/>fbReleaseCode"| FBM

    classDef app fill:#f3e8ff,stroke:#9333ea,color:#581c87;
    classDef esp fill:#dcfce7,stroke:#16a34a,color:#14532d;
    classDef cloud fill:#fff7ed,stroke:#ea580c,color:#7c2d12;
    class APP app;
    class WS,AUTH,FBM,MACHINE esp;
    class FBAUTH,FS1,FS2 cloud;
```

## Notes

- The app **never** talks to Firebase directly — every step above that touches Firebase goes
  through the second board's HTTP server (`WebServer.ino` + `AuthManager.ino` +
  `FirebaseManager.ino`). See [hardware_architecture.md](hardware_architecture.md).
- The HTTP server runs on a separate FreeRTOS core (Core 0) from the session state machine
  (Core 1), so app requests (e.g. checking order history) are answered even while the machine is
  mid-braid.
- Full per-state detail is in [state_machines.md](state_machines.md); full UART message detail is
  in [uart_protocol.md](uart_protocol.md); the emergency path is detailed in [safety.md](safety.md).
