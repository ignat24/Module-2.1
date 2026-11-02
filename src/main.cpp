
#include <Arduino.h>

enum class LedState {
    Off,
    On
};

enum class LedMode {
    Blink,
    AlwaysOn,
    AlwaysOff
};

class CONFIG {
public:
    static constexpr uint8_t LED_PIN = 15;
    static constexpr uint8_t BUTTON_PIN = 0;

    static constexpr uint32_t LED_INTERVAL_MS = 1000;
    static constexpr uint32_t DEBOUNCE_MS = 50;
    static constexpr uint32_t REPORT_ITERATIONS = 1000;

};

class LED {
private:
    uint8_t pin;

public:
    LED(uint8_t ledPin) : pin(ledPin) {}

    void init() {
        pinMode(pin, OUTPUT);
        set(LedState::Off);
    }

    void set(LedState state) {
        digitalWrite(pin, state == LedState::On ? HIGH : LOW);
    }
};

LED led(CONFIG::LED_PIN);

LedState ledState = LedState::Off;
LedMode ledMode = LedMode::Blink;

uint32_t lastLedTime = 0;
uint32_t lastButtonTime = 0;
uint64_t totalLoopTime = 0;
uint32_t minLoopTime = UINT32_MAX;
uint32_t maxLoopTime = 0;
uint32_t loopCount = 0;

volatile bool buttonPressed = false;

void IRAM_ATTR buttonISR() {
    buttonPressed = true;
}

void changeLedMode() {
    switch (ledMode) {
        case LedMode::Blink:
            ledMode = LedMode::AlwaysOn;
            ledState = LedState::On;
            led.set(ledState);
            break;

        case LedMode::AlwaysOn:
            ledMode = LedMode::AlwaysOff;
            ledState = LedState::Off;
            led.set(ledState);
            break;

        case LedMode::AlwaysOff:
            ledMode = LedMode::Blink;
            ledState = LedState::Off;
            led.set(ledState);
            lastLedTime = millis();
            break;
    }
}

void setup() {
    Serial.begin(115200);

    led.init();

    pinMode(CONFIG::BUTTON_PIN, INPUT_PULLUP);

    attachInterrupt(
        digitalPinToInterrupt(CONFIG::BUTTON_PIN),
        buttonISR,
        FALLING
    );
}

void loop() {
    uint32_t startTime = micros();
    uint32_t currentTime = millis();
    bool pressed = false;

    noInterrupts();
    pressed = buttonPressed;
    buttonPressed = false;
    interrupts();


    if (pressed && currentTime - lastButtonTime >= CONFIG::DEBOUNCE_MS) {
        lastButtonTime = currentTime;
        changeLedMode();
    }

    if (ledMode == LedMode::Blink) {
        if (currentTime - lastLedTime >= CONFIG::LED_INTERVAL_MS) {
            lastLedTime = currentTime;

            ledState = (ledState == LedState::Off)
                           ? LedState::On
                           : LedState::Off;
            led.set(ledState);
        }
    }

    uint32_t executionTime = micros() - startTime;
    totalLoopTime += executionTime;
    loopCount++;

    if (executionTime < minLoopTime) {
        minLoopTime = executionTime;
    }

    if (executionTime > maxLoopTime) {
        maxLoopTime = executionTime;
    }

    if (loopCount >= CONFIG::REPORT_ITERATIONS) {
        float averageTime =
            (float)totalLoopTime / loopCount;

        Serial.printf(
            "Loop AVG: %.2f us | MIN: %lu us | MAX: %lu us\n",
            averageTime,
            (unsigned long)minLoopTime,
            (unsigned long)maxLoopTime
        );

        totalLoopTime = 0;
        loopCount = 0;
        minLoopTime = UINT32_MAX;
        maxLoopTime = 0;
    }
}
