#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define LED_PIN 2

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// RTOS Handles
QueueHandle_t commandQueue;

// Shared variable
int currentDelay = 1000;

// Queue structure
struct Command {
  char text[20];
  int blinkRate;
};

// ==========================================
// HEART TASK
// ==========================================
void HeartTask(void *pvParameters) {
  pinMode(LED_PIN, OUTPUT);

  for (;;) {
    digitalWrite(LED_PIN, HIGH);
    vTaskDelay(100 / portTICK_PERIOD_MS);

    digitalWrite(LED_PIN, LOW);
    vTaskDelay(currentDelay / portTICK_PERIOD_MS);
  }
}

// ==========================================
// INPUT TASK
// ==========================================
void InputTask(void *pvParameters) {
  Serial.begin(115200);

  for (;;) {
    if (Serial.available() > 0) {
      String input = Serial.readStringUntil('\n');
      input.trim();

      StaticJsonDocument<200> doc;
      if (deserializeJson(doc, input) == DeserializationError::Ok) {

        Command cmd;

        strlcpy(cmd.text, doc["msg"], sizeof(cmd.text));
        cmd.blinkRate = doc["delay"];

        xQueueSend(commandQueue, &cmd, portMAX_DELAY);

        Serial.println("Command sent.");
      } 
      else {
        Serial.println("JSON Error");
      }
    }

    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}

// ==========================================
// DISPLAY TASK
// ==========================================
void DisplayTask(void *pvParameters) {
  Command receivedCmd;

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 20);
  display.println("Waiting...");
  display.display();

  for (;;) {
    if (xQueueReceive(commandQueue, &receivedCmd, portMAX_DELAY)) {

      currentDelay = receivedCmd.blinkRate;

      display.clearDisplay();
      display.setCursor(0, 20);
      display.println(receivedCmd.text);
      display.display();

      Serial.println("Display updated.");
    }
  }
}

// ==========================================
void setup() {

  commandQueue = xQueueCreate(5, sizeof(Command));

  xTaskCreate(HeartTask, "Heart", 2048, NULL, 1, NULL);
  xTaskCreate(InputTask, "Input", 4096, NULL, 2, NULL);
  xTaskCreate(DisplayTask, "Display", 4096, NULL, 2, NULL);

}

void loop() {}
