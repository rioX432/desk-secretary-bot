# CLAUDE.md

Strictly follow the rules in [AGENTS.md](./AGENTS.md).

## Core Values

1. **ゼロレイテンシの音声対話** — Realtime APIで音声入力→音声応答を直結、自然な会話テンポ
2. **ネット不要で完全ローカル動作** — Module LLMでSTT/LLM/TTS全てオンデバイス実行
3. **デスクに置ける小さなAI秘書** — M5Stack上で動く、常時待機の物理的存在感

## Won't Do
- **スマートホーム制御** — デスク秘書であり、ホームオートメーションハブではない
- **クラウドLLM依存の機能** — Core Value②に反する。ローカル動作が前提
- **汎用ロボティクス機能（移動/マニピュレーション）** — M5Stack上の固定デスクデバイス。動くロボットにしない

## Think Twice

Before acting, always pause and reconsider. Re-read the requirements, re-check your assumptions, and verify your approach is correct before writing any code.

## Research-First Development (No Guessing)

**Guessing is prohibited.** This is an embedded systems project — incorrect assumptions can brick the device or cause hard-to-debug crashes.

1. **Investigate first** — Read official docs (ESP-IDF, M5Unified, Arduino), inspect source code, or web-search to confirm API signatures, memory constraints, and hardware behavior.
2. **Self-review** — After implementing, verify:
   - Consistency with existing patterns in the codebase
   - Memory usage (PSRAM vs heap, stack sizes)
   - No unverified assumptions about hardware behavior
3. **Cross-review with Codex** — If Codex MCP (`mcp__codex__codex`) is available, use it for architecture decisions and code reviews.
4. **Proceed only with confirmed information** — If uncertain, investigate further or ask the user.

## Language

- All code (comments, variable names) must be written in English
- Commit messages: Japanese OK (this is a personal project)
- PR descriptions: Japanese OK

## GitHub

- **Issue/PR 操作は必ず `--repo rioX432/desk-secretary-bot` を指定すること**
- `gh issue` / `gh pr` コマンドをオプションなしで実行すると upstream (`ronron-gh/AI_StackChan_Ex`) を参照してしまう
- 例: `gh issue view 1 --repo rioX432/desk-secretary-bot`

## Key Gotchas

- PlatformIO CLI is at `~/.platformio/penv/bin/pio`, NOT in default PATH
- Build command: `~/.platformio/penv/bin/pio run -e m5stack-cores3` (from `firmware/` directory)
- Upload command: `~/.platformio/penv/bin/pio run -e m5stack-cores3 -t upload`
- SC_SecConfig.yaml contains Wi-Fi passwords and API keys — NEVER commit this file (it's in .gitignore)
- ESP32-S3 has limited heap (~320KB) — use PSRAM (`ps_malloc`) for large buffers
- SPIFFS has ~1.5MB — keep stored files small
- `SpiRamJsonDocument` uses PSRAM for JSON parsing — always use this instead of `DynamicJsonDocument`
- Function Calling loop max iterations: `MAX_REQUEST_COUNT = 10` — avoid infinite tool call loops
- System prompt is stored in SPIFFS `/data.json`, NOT on SD card
- SD card YAML uses the `SC_` prefix convention for all config files
