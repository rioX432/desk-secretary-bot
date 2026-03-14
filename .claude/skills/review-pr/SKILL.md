---
name: review-pr
description: Review a GitHub pull request for desk-secretary-bot
user-invocable: true
disable-model-invocation: true
allowed-tools: Bash, Read, Grep, Glob, Task
---

# PR Code Review

Review the specified pull request (number or URL as argument).

## Steps

1. **Get PR info**: Run `gh pr view <number> --json number,title,body,baseRefName,headRefName,files` to get PR metadata and changed files.
2. **Checkout the branch**: Run `git fetch origin <headRefName> && git checkout <headRefName>` to get the actual code.
3. **Read changed files**: Read each changed file in full to understand context — don't rely on diff alone.
4. **Launch reviewers in parallel**: Based on changed file types, use the Task tool to launch reviewer subagents **in parallel**:
   - `.cpp`/`.h` files under `src/llm/` → `llm-reviewer` agent
   - `.cpp`/`.h` files under `src/` (non-LLM) → `embedded-reviewer` agent
   - `.yaml` files → check for secrets exposure
   - Only launch reviewers for areas that have changes.
5. **Build check**: Run `cd firmware && ~/.platformio/penv/bin/pio run -e m5stack-cores3` to verify compilation.
6. **Aggregate results**: Combine findings from all reviewers and build check. Output a structured review organized by severity (Critical / Important / Suggestion) with `file:line` references.
