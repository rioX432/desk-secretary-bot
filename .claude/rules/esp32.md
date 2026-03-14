# ESP32 / Embedded Rules

## Memory Management
- ESP32-S3 heap is ~320KB — use PSRAM (`ps_malloc`, `ps_calloc`) for buffers >1KB
- Always use `SpiRamJsonDocument` instead of `DynamicJsonDocument` for JSON parsing
- `String` concatenation in loops creates heap fragmentation — use `reserve()` or `snprintf` with char arrays
- Stack size per FreeRTOS task is limited (default 8KB) — avoid large local arrays, use heap allocation

## SPIFFS
- ~1.5MB total — keep stored files small
- System prompt stored at `/data.json`
- Always call `SPIFFS.begin(true)` before file operations
- Check `file.size() > 0` before deserializing (empty file = first boot)

## Wi-Fi / HTTPS
- Use `WiFiClientSecure` with root CA certificates for HTTPS
- Root CAs are in `src/rootCA/` — add new ones there if connecting to new services
- Connection timeout: set explicitly, default can hang indefinitely
- Always check `WiFi.status() == WL_CONNECTED` before HTTP requests

## SD Card (YAML Config)
- All config files use `SC_` prefix convention
- `SC_SecConfig.yaml` — secrets (Wi-Fi, API keys) — NEVER commit
- `SC_BasicConfig.yaml` — hardware (servo pins, LED)
- `SC_ExConfig.yaml` — AI service selection (LLM, TTS, STT type)
- YAML parsing uses `ArduinoYaml` library

## Serial Debug
- Use `Serial.println()` / `Serial.printf()` for debug output
- Monitor at 115200 baud
- Prefix log messages with component name for filtering: `[LLM]`, `[Servo]`, `[MEM]`
