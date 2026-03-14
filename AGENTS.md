# AGENTS.md

This file provides guidance to AI coding agents working with this repository.

## Project Overview

**Desk Secretary Bot** — An autonomous AI desk secretary robot built on the [Stack-chan](https://github.com/stack-chan/stack-chan) platform using M5Stack CoreS3. It combines [AI_StackChan_Ex](https://github.com/ronron-gh/AI_StackChan_Ex) (extended Stack-chan firmware) with autonomous agent capabilities inspired by [MimiClaw](https://github.com/OpenClaw/MimiClaw).

**Goal:** A proactive desk companion that manages schedules, sends reminders, reads emails, and assists daily life through voice interaction.

## Commands

```bash
# Build (from repo root)
cd firmware && ~/.platformio/penv/bin/pio run -e m5stack-cores3

# Upload firmware to CoreS3 (USB-C connected)
cd firmware && ~/.platformio/penv/bin/pio run -e m5stack-cores3 -t upload

# Clean build
cd firmware && ~/.platformio/penv/bin/pio run -e m5stack-cores3 -t clean

# Monitor serial output
cd firmware && ~/.platformio/penv/bin/pio device monitor -b 115200

# Alternative: Use VSCode PlatformIO sidebar
# PROJECT TASKS → m5stack-cores3 → Build / Upload / Monitor
```

## Architecture

### Tech Stack
- **Hardware:** M5Stack CoreS3 (ESP32-S3, 16MB Flash, 8MB PSRAM) + Takao Base + SG90 Servos
- **Framework:** Arduino + ESP-IDF (via PlatformIO, espressif32@6.3.2)
- **LLM:** OpenAI ChatGPT (gpt-4o) with Function Calling
- **STT:** OpenAI Whisper
- **TTS:** OpenAI TTS (tts-1, voice: alloy)
- **Face:** M5Stack Avatar library
- **Servo:** ServoEasing library (SG90 on PortA: GPIO1, GPIO2)

### Source Structure

```
AI_StackChan_Ex/
├── firmware/                    # PlatformIO project
│   ├── platformio.ini           # Build config (multiple board envs)
│   └── src/
│       ├── main.cpp             # Entry point
│       ├── Robot.{cpp,h}        # Robot orchestrator (avatar, servo, audio)
│       ├── llm/
│       │   ├── LLMBase.{cpp,h}  # LLM base class (system prompt, memory, async TTS)
│       │   ├── ChatHistory.{cpp,h}  # Conversation history management
│       │   ├── ChatGPT/
│       │   │   ├── ChatGPT.{cpp,h}      # OpenAI API client + Function Calling loop
│       │   │   ├── FunctionCall.{cpp,h}  # Tool implementations (timer, memo, reminder, etc.)
│       │   │   ├── MCPClient.{cpp,h}     # MCP protocol client
│       │   │   └── RealtimeChatGPT.*     # OpenAI Realtime API (WebSocket)
│       │   ├── Gemini/          # Google Gemini implementation
│       │   └── ModuleLLM*/      # M5Stack Module LLM (local inference)
│       ├── stt/                 # Speech-to-Text implementations
│       ├── tts/                 # Text-to-Speech implementations
│       ├── mod/                 # Application modules (ModBase pattern)
│       ├── driver/              # Hardware drivers
│       ├── Scheduler.{cpp,h}    # Task scheduler
│       ├── WebAPI.{cpp,h}       # Web UI server (personalization, settings)
│       └── SDUtil.{cpp,h}       # SD card config loader
├── Copy-to-SD/                  # Files to copy to SD card
│   ├── yaml/
│   │   ├── SC_BasicConfig.yaml  # Servo pins, hardware config
│   │   └── SC_SecConfig.yaml.example  # Template for Wi-Fi + API keys
│   └── app/AiStackChanEx/
│       └── SC_ExConfig.yaml     # AI service selection (LLM, TTS, STT)
└── doc/                         # Documentation
```

### Key Design Patterns

**LLM Abstraction**
- `LLMBase` → `ChatGPT` / `Gemini` / `ModuleLLM` — polymorphic LLM selection
- System prompt stored in SPIFFS (`/data.json`) with 3 slots: user role, system role, user info (memory)

**Function Calling**
- Max 10 iterations per conversation turn (`MAX_REQUEST_COUNT`)
- Built-in: timer, memo, reminder, date/time, wakeword
- Extension functions (behind `USE_EXTENSION_FUNCTIONS`): email, notes, bus schedule
- MCP tools: dispatched by name through `MCPClient`

**Module System**
- `ModBase` → `AiStackChanMod` — application module pattern
- Lifecycle: `init()` → `update()` loop, button/touch handlers

**Configuration**
- SD card YAML (`SC_*.yaml`) for hardware and service config
- SPIFFS for runtime state (system prompt, memory)
- NVS for persistent key-value storage
- Web UI for personalization (accessible at `http://<device-ip>`)

## Code Quality

- No linter configured (Arduino/ESP32 project) — manually review for:
  - Memory leaks (heap/PSRAM allocations without free)
  - Stack overflow risks (large local variables in tasks)
  - String concatenation in loops (use `reserve()` or `snprintf`)
  - Blocking operations on Core 0 (audio task)

## Git Commits

- Keep commit messages concise (one line preferred)
- Do NOT add AI stamps or Co-Authored-By lines
- Reference GitHub Issues with `refs #N` or `closes #N`

## SD Card Setup

1. Format microSD as FAT32 (32GB or less)
2. Copy `Copy-to-SD/yaml/` → SD root `/yaml/`
3. Copy `Copy-to-SD/app/` → SD root `/app/`
4. Create `yaml/SC_SecConfig.yaml` from `.example` template with real credentials
