# LLM Integration Rules

## Architecture
- `LLMBase` is the abstract base class — all LLM implementations extend it
- System prompt has 3 slots: `USER_ROLE` (persona), `SYSTEM_ROLE` (behavior rules), `USER_INFO` (memory)
- System prompt persists in SPIFFS `/data.json` — survives reboots

## Function Calling
- Max iterations per turn: `MAX_REQUEST_COUNT = 10` — prevent infinite loops
- Built-in functions defined in `FunctionCall.cpp` — register new ones there
- Extension functions require `USE_EXTENSION_FUNCTIONS` build flag
- MCP tools: dispatched by name through `MCPClient` — no code change needed for new MCP tools
- Function result must fit in response buffer — keep results concise

## Chat History
- `ChatHistory` manages conversation context (max 20 turns)
- Older messages are dropped FIFO — important context should be in system prompt or memory
- `chat_doc` (SpiRamJsonDocument) holds the full API request — monitor its size

## Async TTS
- LLM response text is queued via `outputTextQueue` for async TTS playback
- Text is split at delimiters (。？！) for natural speech pacing
- `speaking` flag prevents overlapping audio

## API Keys
- All API keys come from `SC_SecConfig.yaml` on SD card
- Same OpenAI key is used for LLM (ChatGPT), STT (Whisper), and TTS
- Keys are passed via `llm_param_t` struct at initialization
