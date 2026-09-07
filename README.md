
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
@article{,
  title={Securing Marine Assets: Edge ML and LoRa Mesh Integration for IoT Anti-Theft Systems},
  author={},
  journal={},
  year={2026},
  volume={XX},
  pages={XXX-XXX},
  doi={10.XXXX/XXXX}
}
