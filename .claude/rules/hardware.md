# Hardware Rules

## CoreS3
- ESP32-S3 with 16MB Flash, 8MB PSRAM
- Built-in: 2" touchscreen (320x240), camera (GC0308), mic (ES7210), speaker (NS4168), IMU (BMI270)
- PortA (I2C/GPIO): GPIO1 (SDA), GPIO2 (SCL) — used for servo on Takao Base
- USB-C for power and programming

## Servo (SG90)
- Pan (horizontal): GPIO1 via PortA
- Tilt (vertical): GPIO2 via PortA
- Takao Base: `takao_base: true` in SC_BasicConfig.yaml
- ServoEasing library for smooth movement
- Range: 0-180° — center at 90°
- Do NOT command rapid back-and-forth — servos have mechanical limits

## Avatar (Face Display)
- M5Stack Avatar library renders face on LCD
- Expressions: happy, angry, sad, sleepy, doubt, neutral
- Lip sync driven by TTS audio amplitude
- Avatar runs on Core 0, LLM/network on Core 1

## Audio
- Mic: ES7210 I2S — used for STT recording
- Speaker: NS4168 I2S — used for TTS playback
- Audio processing on Core 0 (high priority)
- Do NOT block Core 0 with long operations — causes audio glitches
