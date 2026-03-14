---
name: build
description: Build and optionally flash firmware to CoreS3
argument-hint: "[build|upload|monitor|clean]"
user-invocable: true
disable-model-invocation: true
allowed-tools:
  - Bash(cd firmware && ~/.platformio/penv/bin/pio *)
  - Bash(~/.platformio/penv/bin/pio *)
---

# /build — Build & Flash Firmware

**Action:** "$ARGUMENTS" (default: build)

## Commands

| Argument | Action |
|----------|--------|
| `build` (default) | Compile firmware for CoreS3 |
| `upload` | Build and flash to connected CoreS3 |
| `monitor` | Open serial monitor (115200 baud) |
| `clean` | Clean build artifacts |

## Execution

```bash
cd firmware && ~/.platformio/penv/bin/pio run -e m5stack-cores3 {target}
```

Where `{target}` is:
- `build` → (no target flag)
- `upload` → `-t upload`
- `monitor` → `device monitor -b 115200`
- `clean` → `-t clean`

## Post-Build Check

After build, report:
- **Status**: SUCCESS / FAILED
- **RAM usage**: XX% (warn if >80%)
- **Flash usage**: XX% (warn if >90%)
- **Build time**: XX seconds
- **Warnings**: List any new warnings in modified files
