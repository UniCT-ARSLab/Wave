#include "autoencoder_data.h"
#include <Arduino.h>
#include <EloquentTinyML.h>
#include <Preferences.h>
#include <Wire.h>
#include <iot_board.h>
#include <math.h>

bool checking;
uint32_t anomaly_counter = 0;
int checking_counter = WINDOW_SIZE;

Eloquent::TinyML::TfLite<TF_NUM_INPUTS, TF_NUM_OUTPUTS, TENSOR_ARENA_SIZE> ml;
float input_buffer[TF_NUM_INPUTS] = {0};
float output_buffer[TF_NUM_OUTPUTS] = {0};
int counter_input = 0;
float dinamic_threshold = ANOMALY_THRESHOLD;
const float ALPHA = 0.05;
const float THRESHOLD_MULTIPLIER = 1.5;
float ema_mse = -1.0;

// giroscopio
#define LSM6DSO_ADDR 0x6B
#define CTRL2_G 0x11 // Registro CTRL2_G (indirizzo 0x11 per l'LSM6DSO)
#define G_CONFIG_VAL                                                           \
  0x4C // 104 Hz (ODR_G = 0100) + 2000 dps (FS_G = 11) -> 0x4C
float offset_x = 0.0, offset_z = 0.0;
const float DEADBAND = 1.0;

// magnetometro
#define LIS2MDL_ADDR 0x1E // Indirizzo I2C del LIS2MDL (magnetometro)*
#define CFG_REG_A 0x60    // Registro CFG_REG_A (indirizzo 0x60 per il LIS2MDL)
#define M_CONFIG_VAL                                                           \
  0x8C // 104 Hz (ODR_G = 0100) + 2000 dps (FS_G = 11) -> 0x4C
float quiet_direction = 0.0;

float mse = 0.0f;

bool runEvery(
    unsigned long interval) { // serve per eseguire un blocco di codice
                              // ogni tot millisecondi senza sleep
  static unsigned long previousMillis = 0;
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    return true;
  }
  return false;
}

void displayPrintln(String text) {
  display->clearDisplay();
  display->setCursor(0, 0);
  display->println(text);
  display->display();
}
void displayAppend(String text) {
  display->println(text);
  display->display();
}

// l'errore quadratico medio (MSE)
float runInference(float *input, float *output) {
  float normalized_input[TF_NUM_INPUTS];

  for (int i = 0; i < TF_NUM_INPUTS; i += 2) {
    normalized_input[i] = (input[i] - SCALER_MEAN_X) / SCALER_SCALE_X;
    normalized_input[i + 1] = (input[i + 1] - SCALER_MEAN_Y) / SCALER_SCALE_Y;
  }

  ml.predict(normalized_input, output);

  float mse = 0.0f;
  for (int i = 0; i < TF_NUM_INPUTS; i++) {
    float diff = normalized_input[i] - output[i];
    mse += diff * diff;
  }

  return mse / TF_NUM_INPUTS;
}

// per calcolare media e deviazione standard di un array di float
float avg(float *data, int size) {
  float sum = 0.0f;
  for (int i = 0; i < size; i++) {
    sum += data[i];
  }
  return sum / size;
}

float std_dev(float *data, int size) {
  float mean = avg(data, size);
  float sum = 0.0f;
  for (int i = 0; i < size; i++) {
    float diff = data[i] - mean;
    sum += diff * diff;
  }
  return sqrt(sum / size);
}

void calibrateGyro() {
  displayPrintln("CALIBRAZIONE...");
  displayAppend("Non muovere!");
  display->display();

  long sum_x = 0, sum_z = 0;
  for (int i = 0; i < 100; i++) {
    sum_x += readGiroData(0x22);
    sum_z += readGiroData(0x26);
    delay(10);
  }
  offset_x = (sum_x / 1000.0) * 0.070;
  offset_z = (sum_z / 1000.0) * 0.070;
}

int16_t readGiroData(byte reg) {
  Wire.beginTransmission(LSM6DSO_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(LSM6DSO_ADDR, 2);
  return Wire.read() | (Wire.read() << 8);
}

int16_t readMagData(byte reg) {
  Wire.beginTransmission(LIS2MDL_ADDR);
  Wire.write(reg); // Indirizzo del registro
  Wire.endTransmission(false);
  Wire.requestFrom(LIS2MDL_ADDR, 2); // Leggi 2 byte (byte basso e alto)
  return Wire.read() | (Wire.read() << 8);
}

void init_autoencoder() {
  if (!ml.begin(autoencoder_data)) {           // Inizializza il modello
    displayPrintln(String(ml.errorMessage())); // Stampa l'errore sul display
    displayAppend(
        String(ESP.getFreeHeap())); // Stampa la memoria libera sul display
  } else {
    ml.turnInputScalingOn();
    ml.turnOutputScalingOn();
  }
  displayPrintln("Modello caricato correttamente (autoencoder_data)");

  Wire.begin();
  Wire.beginTransmission(LSM6DSO_ADDR);
  Wire.write(CTRL2_G);      // Seleziona il registro
  Wire.write(G_CONFIG_VAL); // Invia la configurazione
  Wire.endTransmission();

  Wire.beginTransmission(LIS2MDL_ADDR);
  Wire.write(CFG_REG_A);    // Seleziona il registro
  Wire.write(M_CONFIG_VAL); // Invia la configurazione
  Wire.endTransmission();

  calibrateGyro();
}

bool is_there_anomaly(bool calibration) {
  if (runEvery(200)) {
    display->clearDisplay();

    float gx = (readGiroData(0x22) * 0.070) - offset_x;
    float gz = (readGiroData(0x26) * 0.070) - offset_z;

    quiet_direction = atan2(readMagData(0x6A), readMagData(0x68)) *
                      (180.0 / PI); // Converti in gradi
    if (quiet_direction < 0)
      quiet_direction += 360.0; // Assicurati che sia positivo

    displayPrintln("GYRO_X: " + String(gx) + " dps" +
                   "\nGYRO_Z: " + String(gz) + " dps" +
                   "\nMAG_DIR: " + String(quiet_direction) + " deg");

    for (int i = 0; i < TF_NUM_INPUTS - 2; i++)
      input_buffer[i] = input_buffer[i + 2];
    input_buffer[TF_NUM_INPUTS - 2] = gx;
    input_buffer[TF_NUM_INPUTS - 1] = gz;

    if (counter_input < (TF_NUM_INPUTS / 2)) {
      digitalWrite(LED_YELLOW, HIGH);
      counter_input++;
    } else {
      digitalWrite(LED_YELLOW, LOW);
      mse = runInference(input_buffer, output_buffer);

      displayAppend("MSE: " + String(mse, 6) +
                    "\nTHRESHOLD: " + String(dinamic_threshold, 6) + "\n" +
                    (mse > dinamic_threshold ? " (ANOMALY)" : " (NORMAL)"));

      ema_mse = (ALPHA * mse) + ((1.0 - ALPHA) * ema_mse);
      dinamic_threshold = ema_mse * THRESHOLD_MULTIPLIER;

      if (ema_mse < 0) {
        ema_mse = mse;
        dinamic_threshold = ANOMALY_THRESHOLD;
      }

      if (mse > dinamic_threshold && checking == false) {
        checking_counter = WINDOW_SIZE;
        anomaly_counter = 0;
        checking = true;
        anomaly_counter++;
      }

      if (checking && checking_counter > 0) {
        if (mse > dinamic_threshold) {
          anomaly_counter++;
        }
        checking_counter--;

        if (anomaly_counter > WINDOW_SIZE / 2) {
          return true;
        }

      } else {
        checking = false;
      }
    }
  }
  return false;
}

int timer_calibration = 20;

void print_data() {
  if (runEvery(200)) {
    float gx = (readGiroData(0x22) * 0.070) - offset_x;
    float gz = (readGiroData(0x26) * 0.070) - offset_z;

    Serial.print(millis()); // Timestamp
    Serial.print(",");
    Serial.print(gx, 3); // 3 cifre decimali
    Serial.print(",");
    Serial.println(gz, 3);

    if (timer_calibration > 0)
      timer_calibration--;
    // else
    // calibration_on = false;
  }
}
