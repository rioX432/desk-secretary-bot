---
name: dev
description: "End-to-end: investigate → implement → build → flash → PR"
argument-hint: "[GitHub issue #, e.g. #7]"
user-invocable: true
disable-model-invocation: true
allowed-tools:
  - Bash(git checkout:*)
  - Bash(git add:*)
  - Bash(git commit:*)
  - Bash(git push:*)
  - Bash(git diff:*)
  - Bash(git log:*)
  - Bash(git status)
  - Bash(git branch:*)
  - Bash(cd firmware && ~/.platformio/penv/bin/pio *)
  - Bash(~/.platformio/penv/bin/pio *)
  - Bash(gh pr create --repo rioX432/desk-secretary-bot:*)
  - Bash(gh issue view --repo rioX432/desk-secretary-bot:*)
  - Bash(gh issue comment --repo rioX432/desk-secretary-bot:*)
  - Glob
  - Grep
  - Read
  - Edit
  - Write
  - Task
  - TaskCreate
  - TaskUpdate
  - TaskList
  - ToolSearch
  - AskUserQuestion
  - mcp__codex__codex
  - mcp__codex__codex-reply
---

# /dev — End-to-End Development Workflow

Execute the full development cycle for an issue: investigate → implement → build → (flash) → PR.

**Target:** "$ARGUMENTS"

## Setup: Create Task Tracker

Use `TaskCreate` to create a task for each phase:
1. "Gather context from issue"
2. "Investigate codebase"
3. "Implement changes"
4. "Build firmware"
5. "Commit changes"
6. "Create PR"

Use `TaskUpdate` to mark each task `in_progress` when starting and `completed` when done.

---

## Phase 1: Context Gathering

### 1a. Issue Context

**GitHub Issue** (starts with `#`):
1. Run `gh issue view <number> --repo rioX432/desk-secretary-bot --json number,title,body,labels`
2. Extract: title, description, acceptance criteria, labels

**Branch naming** (from labels or issue type):
- `phase-N` label → branch prefix matching phase
- Bug → branch prefix `fix/`
- Feature → branch prefix `feature/`
- Format: `{prefix}/{issue-number}-{kebab-case-short-description}`

### 1b. Project Context

Read these files (use Read tool directly):
- `AGENTS.md` — architecture, commands, and conventions
- Relevant `.claude/rules/` files based on the issue topic

---

## Phase 2: Investigation

Use the `Task` tool with `subagent_type: "Explore"` and thoroughness `"very thorough"` to investigate the codebase.

The subagent must:

1. **Find relevant code**: Use Grep/Glob to locate files matching the issue context
2. **Read the code**: Actually read every file involved — no speculation allowed
3. **Trace the flow**: Follow the component chain (main.cpp → Robot → LLM → FunctionCall)
4. **Impact analysis**: List files that need changes, callers, downstream dependencies

### No Speculation Principle
- Every finding must be backed by actual code reading
- This is embedded C++ — incorrect assumptions cause hard crashes
- If unsure, read the actual hardware driver code

### Unclear Points
If anything is ambiguous, use `AskUserQuestion` to ask the user. Do NOT proceed with assumptions.

---

## Phase 3: Cross-Check (Codex)

If Codex MCP (`mcp__codex__codex`) is available:
1. Call `mcp__codex__codex` with investigation results and proposed approach
2. Ask Codex to verify: root cause, impact, memory implications, hardware safety

If unavailable: Skip and note it was skipped.

---

## ── AskUserQuestion: Approach Confirmation ──

Present to the user:
1. **Root cause / Implementation plan** (with `file:line` references)
2. **Impact** (files affected, memory budget impact)
3. **Proposed approach**
4. **Codex verification** (if done)

Ask the user to confirm before proceeding.

---

## Phase 4: Branch & Implement

### 4a. Create Branch

```bash
git checkout -b {branch-name}
```

### 4b. Implement

**Guidelines:**
- Follow existing code patterns (read surrounding code first)
- Follow AGENTS.md conventions
- Keep changes minimal — no unnecessary refactoring
- Use `SpiRamJsonDocument` for JSON, `ps_malloc` for large buffers
- Add `Serial.printf` debug logging for new features
- Keep Function Call results concise (ESP32 buffer limits)

---

## Phase 5: Build

```bash
cd firmware && ~/.platformio/penv/bin/pio run -e m5stack-cores3
```

Check for:
- `SUCCESS` in output
- RAM usage (<80% recommended)
- Flash usage (<90% recommended)
- No warnings in new/modified code

### Failure Handling
- Max 3 fix attempts for build errors
- If still failing after 3 tries, report to user and stop

---

## Phase 6: Commit

### ── AskUserQuestion: Commit Confirmation ──

Show the user:
1. Summary of changes (files modified/created)
2. Build result (RAM/Flash usage)
3. Proposed commit message

Commit message format:
- Concise, one line preferred
- No Co-Authored-By, no AI stamps
- Reference issue: `refs #N` or `closes #N`

Only after user confirmation:

```bash
git add {specific files}
git commit -m "{message}"
```

---

## Phase 7: PR Creation

### ── AskUserQuestion: PR Creation Confirmation ──

Show the user branch name, PR title, and target branch. Ask to confirm.

Only after confirmation:

```bash
git push -u origin {branch-name}
```

```bash
gh pr create --repo rioX432/desk-secretary-bot --title "{PR title}" --body "$(cat <<'EOF'
## Description

- {bullet point summary}

## Related Issues

{Closes #N}

## Test Plan

- [x] {how it was tested — build success, serial monitor, hardware test}

## Review Checklist

- [x] Memory usage checked (heap/PSRAM within budget)
- [x] No secrets committed (API keys, Wi-Fi passwords)
- [x] Tested on CoreS3 hardware (or build-only if no device)
- [x] Existing features not broken

## Breaking Changes

None
EOF
)"
```

Print the PR URL.

---

## Error Handling

| Situation | Action |
|-----------|--------|
| Issue not found | Report error, stop |
| Investigation unclear | AskUserQuestion before proceeding |
| Codex unavailable | Skip, note it was skipped |
| Build fails (≤3 attempts) | Fix and retry |
| Build fails (>3 attempts) | Report to user, stop |
| RAM/Flash >90% | Warn user before proceeding |
