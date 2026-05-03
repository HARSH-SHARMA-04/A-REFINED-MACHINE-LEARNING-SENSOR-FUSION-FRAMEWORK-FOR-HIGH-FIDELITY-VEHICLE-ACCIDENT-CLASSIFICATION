# 🚗 A Refined Machine Learning Sensor Fusion Framework for High-Fidelity Vehicle Accident Classification

> **B.Tech Computer Science Engineering — Capstone Project**
> DIT University, Dehradun, India | May 2026

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
![Python](https://img.shields.io/badge/Python-3.8%2B-blue)
![TensorFlow](https://img.shields.io/badge/TensorFlow-2.x-orange)
![scikit-learn](https://img.shields.io/badge/scikit--learn-latest-green)
![Accuracy](https://img.shields.io/badge/Best%20Accuracy-98.84%25-brightgreen)

---

## 👥 Authors

| Name | Roll No. |
|------|----------|
| Harsh Sharma | 220102157 |
| Devraj Shukla | 220102862 |
| Amit Kumar | 220102849 |

**Supervisor:** Mr. Tarun Kumar, Assistant Professor, DIT University

---

## 📌 Overview

Road accidents claim over **1.35 million lives annually** worldwide. This project presents a complete end-to-end ML-based sensor fusion framework that can automatically detect and classify vehicle accident events in real time using low-cost embedded hardware.

The system classifies three event types:
- 🔴 **Collision** — vehicle impact with a rigid object
- 🟡 **Fall-Off** — vehicle falling from an elevated surface
- 🟢 **Normal Driving** — regular driving with no accident

A stacking ensemble of four ML classifiers achieves a best accuracy of **98.84%** on a 9,070-sample real sensor dataset.

---

## 🏗️ System Architecture

```
Sensors (GY-87 + NEO-6M GPS)
        ↓
ESP8266 WiFi Module
        ↓
Arduino IDE (Data Collection)
        ↓
CSV Dataset (9,070 samples)
        ↓
Signal Preprocessing (Butterworth Filter + Complementary Filter)
        ↓
Feature Engineering (17 features: ALA, Jerk, Delta_Altitude, ...)
        ↓
ADASYN Augmentation (Training Set Only)
        ↓
ML Classifiers: ANN | SVM | Decision Tree | Naive Bayes
        ↓
Stacking Ensemble (Logistic Regression Meta-Learner)
        ↓
Classification Output: Collision / Fall-Off / Normal Driving
```

---

## 🔧 Hardware Components

| Sensor | Model | Purpose |
|--------|-------|---------|
| IMU (Accel + Gyro + Baro + Mag) | GY-87 (MPU-6050, BMP180, HMC5883L) | Motion, orientation, altitude |
| GPS Module | NEO-6M (u-blox) | Speed and position |
| WiFi Transceiver | ESP8266 | Wireless data transmission |
| Microcontroller Platform | Arduino IDE (ESP8266) | Firmware and data logging |

---

## 📊 Dataset

| Class | Samples | % of Dataset |
|-------|---------|-------------|
| Normal Driving | 5,641 | 62.2% |
| Fall-Off | 2,213 | 24.4% |
| Collision | 1,216 | 13.4% |
| **Total** | **9,070** | **100%** |

- **17 numeric input features** per sample
- Collected using a real RC model vehicle in controlled experiments
- Class imbalance addressed using **ADASYN** augmentation

---

## 🤖 ML Models & Results

| Model | Accuracy (No Aug) | Accuracy (+ADASYN) | F1 (No Aug) | F1 (+ADASYN) | Rank |
|-------|:-----------------:|:------------------:|:-----------:|:------------:|:----:|
| **Stacking Ensemble** | — | **98.84%** | — | **0.9885** | 🥇 1st |
| ANN | 98.46% | 98.73% | 0.9846 | 0.9874 | 🥈 2nd |
| SVM (RBF) | 95.42% | 96.47% | 0.9562 | 0.9658 | 🥉 3rd |
| Decision Tree | 97.19% | 95.64% | 0.9723 | 0.9579 | 4th |
| Naive Bayes | 91.95% | 89.36% | 0.9195 | 0.8963 | 5th |

### Key Insight — Top 3 Features (all models agree):
```
1. Delta_Altitude  →  Primary discriminator for Fall-Off (importance: 0.292–0.358)
2. Jerk            →  Primary discriminator for Collision (importance: 0.097–0.135)
3. Acc_Mag         →  Secondary Collision signal        (importance: 0.028–0.060)
```

---

## 🧪 Feature Engineering

17 features extracted from raw sensor streams:

| # | Feature | Source | Role |
|---|---------|--------|------|
| 1–3 | AccX, AccY, AccZ | Accelerometer | Linear dynamics |
| 4 | Acc_Mag | Accelerometer | Impact severity |
| 5–7 | GyroX, GyroY, GyroZ | Gyroscope | Rotational motion |
| 8 | Gyro_Mag | Gyroscope | Rotational magnitude |
| 9–11 | Pitch, Roll, Yaw | Complementary Filter | Vehicle orientation |
| 12–13 | Delta_Pitch, Delta_Roll | Derived | Tilt onset |
| 14 | **Jerk** | Derived | **Impact spike detector** |
| 15 | Altitude | Barometer | Absolute elevation |
| 16 | **Delta_Altitude** | Barometer | **Fall-Off detector** |
| 17 | Speed | GPS | Vehicle velocity |

---

## ⚙️ Signal Preprocessing

- **4th-order Butterworth Low-Pass Filter** (cutoff: 10 Hz) — removes motor vibration noise
- **Complementary Filter** — fuses accelerometer + gyroscope for stable Euler angle estimation (98% gyro / 2% accelerometer weighting)
- **Z-score Normalization** — StandardScaler fitted on training set only
- **ADASYN Oversampling** — adaptive synthetic sampling for minority class augmentation (training set only)

---

## 🏗️ Stacking Ensemble Architecture

```
Base Learners (trained on ADASYN-augmented data):
├── ANN         → 3 class probabilities
├── SVM (RBF)   → 3 class probabilities
├── Decision Tree → 3 class probabilities
└── Naive Bayes (trained on original data + PowerTransformer) → 3 class probabilities
                         ↓
          12-feature meta-input vector
                         ↓
     Meta-Learner: Logistic Regression (5-fold stratified CV)
                         ↓
              Final Classification Output
```

---

## 🚀 Getting Started

### Prerequisites

```bash
pip install numpy pandas scikit-learn tensorflow keras imbalanced-learn matplotlib seaborn
```

### Clone the Repository

```bash
git clone https://github.com/HARSH-SHARMA-04/A-REFINED-MACHINE-LEARNING-SENSOR-FUSION-FRAMEWORK-FOR-HIGH-FIDELITY-VEHICLE-ACCIDENT-CLASSIFICATION.git
cd A-REFINED-MACHINE-LEARNING-SENSOR-FUSION-FRAMEWORK-FOR-HIGH-FIDELITY-VEHICLE-ACCIDENT-CLASSIFICATION
```

### Run the Notebook

Open the project in **Google Colaboratory** or locally:

```bash
jupyter notebook ML_Sensor_Fusion_Accident_Classification.ipynb
```

### Project Structure

```
📦 project-root/
┣    📄 CAR_DATA_RAW_VALUES.csv                                  # Raw multi-sensor CSV files
┣    📄 ACCIDENT CLASSIFICATION FINAL DATASET.csv                # Final merged 9070-sample datase
┣    📄 Arduino IDE CODE for Data Collection.ino                 # ESP8266 firmware for data collection
┣    📄 Final_Code.ipynb                                         # Main Jupyter notebook
┣    📄 requirements.txt
┣    📄 LICENSE
┣    📄 README.md
```

---

## 📈 3-Step Rule-Based Classifier (91% Accuracy — No ML Required)

For lightweight/embedded deployment:

| Step | Condition | Output |
|------|-----------|--------|
| 1 | `Delta_Altitude ≥ 0.5 m` | **FALL OFF** |
| 2 | `Jerk ≥ 4.0` OR `Acc_Mag ≥ 20g` | **COLLISION** |
| 3 | Otherwise | **NORMAL DRIVING** |

---

## 🔮 Future Work

- Extended classification taxonomy (Rollover, Aquaplaning, Rear-End Pile-Up)
- Deep learning on raw sequences (LSTM, 1D-CNN, Transformer)
- Real full-scale vehicle deployment
- V2X emergency dispatch integration
- GPS-loss fallback strategies for tunnels/forests

---

## 📄 License

This project is licensed under the **MIT License** — see the [LICENSE](LICENSE) file for details.

---

## 🙏 Acknowledgements

We thank **Mr. Tarun Kumar** (Assistant Professor, DIT University) for his continuous guidance and support throughout this project.

---

## 📚 Key References

- Kumar, N., Acharya, D., & Lohani, D. (2020). An IoT Based Vehicle Accident Detection and Classification System using Sensor Fusion.
- He, H., et al. (2008). ADASYN: Adaptive synthetic sampling for imbalanced learning. IEEE IJCNN.
- Chawla, A., et al. (2018). Multi-sensor fusion for vehicle fall detection on mountain roads. Sensors, MDPI.
- Wolpert, D. H. (1992). Stacked generalization. Neural Networks, 5(2), 241–259.

---

<p align="center">Made with ❤️ at DIT University, Dehradun | May 2026</p>
