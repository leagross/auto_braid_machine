//  Config.h 
#ifndef CONFIG_H                 
#define CONFIG_H

// ---------- כפתור התחלה / עצירת חירום ----------
#define BUTTON_PIN   27   

// ---------- חיישן אולטרסוני (HC-SR04) ----------
#define TRIG_PIN     21             // רגל TRIG (יציאה — שולחת פולס)
#define ECHO_PIN     19             // רגל ECHO (כניסה — מודדת הד)
#define DIST_MIN_CM  7.0            // מרחק מינימלי תקין (ס"מ)
#define DIST_MAX_CM  16.0           // מרחק מקסימלי תקין (ס"מ)

// ---------- חיישן צבע (TCS34725, I2C) ----------
#define COLOR_SDA    32             // I2C SDA
#define COLOR_SCL    26             // I2C SCL
#define COLOR_LED_PIN 33            // שליטה על נורת ה-LED של החיישן (דלוקה רק בזמן מדידה)

// ---------- מנוע תוספות (ULN2003 / 28BYJ-48) ----------
#define DISP_IN1  2
#define DISP_IN2  15
#define DISP_IN3  18
#define DISP_IN4  5

#define STEP_DELAY_MS      3        // השהיה בין פסיעות (מהירות/מומנט)
#define DISP_STEPS_PER_REV 2048     // פסיעות לסיבוב מלא של הקרוסלה (28BYJ-48)
#define DISP_QUARTER_STEPS (DISP_STEPS_PER_REV / 4) 

// ---------- UART אל הלוח השני (Serial2 על פינים רגילים 16/17) ----------
#define UART_TX_PIN  17             // GPIO17 = TX
#define UART_RX_PIN  16             // GPIO16 = RX
#define UART_BAUD    115200        

// ---------- כללי ----------
#define BTN_DEBOUNCE_MS 50          // דיבאונס לכפתור (מ"ש)

#endif 
