//  Config.h — pins and constants for the "senses" board (first_esp32)
#ifndef CONFIG_H
#define CONFIG_H

// ---------- Start / emergency-stop button ----------
#define BUTTON_PIN   27

// ---------- Ultrasonic sensor (HC-SR04) ----------
#define TRIG_PIN     21             // TRIG pin (output — sends the pulse)
#define ECHO_PIN     19             // ECHO pin (input — measures the echo)
#define DIST_MIN_CM  7.0            // minimum valid distance (cm)
#define DIST_MAX_CM  16.0           // maximum valid distance (cm)

// ---------- Color sensor (TCS34725, I2C) ----------
#define COLOR_SDA    32             // I2C SDA
#define COLOR_SCL    26             // I2C SCL
#define COLOR_LED_PIN 33            // controls the sensor's LED (only on during a measurement)

// ---------- Extension dispenser motor (ULN2003 / 28BYJ-48) ----------
#define DISP_IN1  2
#define DISP_IN2  15
#define DISP_IN3  18
#define DISP_IN4  5

#define STEP_DELAY_MS      3        // delay between steps (speed/torque tradeoff)
#define DISP_STEPS_PER_REV 2048     // steps per full revolution of the carousel (28BYJ-48)
#define DISP_QUARTER_STEPS (DISP_STEPS_PER_REV / 4)

// ---------- UART to the second board (Serial2 on regular pins 16/17) ----------
#define UART_TX_PIN  17             // GPIO17 = TX
#define UART_RX_PIN  16             // GPIO16 = RX
#define UART_BAUD    115200

// ---------- General ----------
#define BTN_DEBOUNCE_MS 50          // button debounce (ms)

#endif
