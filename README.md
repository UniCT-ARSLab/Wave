
# Wave 🌊

> **Official Prototype Repository of:**  
> *"Securing Marine Assets: Edge ML and LoRa Mesh Integration for IoT Anti-Theft Systems"*

**Project Status: Prototype**  
*This repository contains the proof-of-concept firmware and machine learning model used for the research paper.*

## 📖 Overview

**Wave** is an experimental IoT anti-theft system designed for the marine environment. Protecting marine assets (boats) presents unique challenges due to lack of cellular coverage and constant environmental movement (waves, wind). 

This system solves these challenges by combining:
1. **Edge Machine Learning:** Local anomaly detection (e.g., distinguishing natural wave motion from tampering or theft attempts) running directly on the microcontroller.
2. **LoRa Mesh Networking:** A decentralized, low-power, long-range communication protocol to relay alerts back to shore without relying on cellular networks.

## 🗂️ Repository Structure

*   `/Firmware` - contains the pio project.
*   `/ml_models` - Training datasets, Jupyter notebooks for feature extraction, and the exported Edge ML models.

## 🛠️ System Requirements

### Hardware Used
*   **Microcontroller: Arduino Nano ESP32** 
*   **LoRa Radio: Semtech SX1276**
*   **Sensors: X-NUCLEO-IKS01A3**

## Getting Started 

### 1. Training the Model
The Firmware already contains the weights of a trained autoencoder inside: `/Firmware/lib/autoencoder/autoencoder.h` 

If you would like to train your own, follow these steps:  
1. Navigate to the `/ml_models` directory.
2. Install Python dependencies: `pip install -r requirements.txt`.

### 2. Flashing the Boat Nodes
1. Open the `Firmware/lib/` folder in PlatformIO.
2. Update the `state.h` file with your specific node ID.
3. Build and upload the firmware to your microcontroller.

## 📝 Citation

If you use this code, data, or architecture in your research, please cite our paper:

```bibtex

@Article{iot7030079,
AUTHOR = {Coppola, Damiano Vincenzo and Russo, Miriana and Santoro, Corrado and Santoro, Federico Fausto and Spadola, Angelo and Tudisco, Alessio},
TITLE = {Securing Marine Assets: Edge ML and LoRa Mesh Integration for IoT Anti-Theft Systems},
JOURNAL = {IoT},
VOLUME = {7},
YEAR = {2026},
NUMBER = {3},
ARTICLE-NUMBER = {79},
URL = {https://www.mdpi.com/2624-831X/7/3/79},
ISSN = {2624-831X},
ABSTRACT = {This paper presents a maritime Internet of Things anti-theft architecture based on an ESP32 onboard node, local motion analysis, and LoRa mesh communication. The proposed detection pipeline uses a one-dimensional convolutional autoencoder trained on stationary vessel data and applied to sliding windows of X- and Z-axis angular velocity measurements. During deployment, a calibration phase estimates the reconstruction-error threshold from the local motion profile of the moored vessel, reducing the dependence on labelled theft examples. The communication layer combines an Elliptic Curve Cryptography setup phase with symmetric payload encryption for alert packets, while ESP32 hardware security features are used to protect firmware and stored credentials. The mesh network uses controlled flooding. A Godot-based simulation environment was used to generate stationary and towing scenarios under different wave configurations. In the current simulation campaign, towing windows produced a higher mean reconstruction error than stationary windows. The results support the feasibility of the architecture and also show that event-level alert logic is required to aggregate window-level anomaly scores into reliable alarms.},
DOI = {10.3390/iot7030079}
}
