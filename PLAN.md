# CyberpunkMP revival — CP2077 patch 2.31a (GOG/Heroic/Proton)

**Original request (verbatim):** "plan this complete thing out, use workflows of opus 4.8 and sonnet 5 to safe on token usage and build the thing. you build the plan and opus/sonnet build it"

**Goal:** Fork of tiltedphoques/CyberpunkMP building and running against Cyberpunk 2077 v2.31a
(user's install: `~/Games/Heroic/Cyberpunk 2077`, GOG via Heroic, Proton).
Orchestrator = Fable (plan/verify/gnarly calls); builders = Opus 4.8 + Sonnet 5 workflow agents.

## Facts established (session 2026-07-12)
- Workspace: `~/CyberpunkMP`, branch `patch-2.31a`, origin = blyatiful1/CyberpunkMP (fork), upstream = tiltedphoques.
- Upstream CI: `windows.yml` builds client on windows-latest (MSVC+xmake 2.9.6), uploads packaged mod as
  artifact `build/xpack/Cyberpunk Multiplayer/Artifacts.zip`. `linux.yml` builds server on ubuntu-24.04.
  → Client DLL can be built by GitHub Actions on our fork; gh CLI authenticated (repo+workflow scopes).
- Vendored submodules (only RED4ext.SDK is *compiled* into client): RED4ext.SDK @ f52af6a (tiltedphoques fork),
  Codeware v1.13.0, ArchiveXL v1.18.1-beta-6, TweakXL v1.10.6.
- Local box: no xmake/dotnet yet (pacman has xmake 3.0.9, dotnet-sdk-9.0); gcc 16 / clang 22; wine 11.12; 462 GB free.
- Known upstream issues: #53 GameNetworkingSockets build failure; README warns WinSDK must be < 10.0.26100 (protobuf-cpp).

## Milestones (each with a runnable end-check)
- [ ] M0  Toolchain: `xmake config --yes` succeeds in repo on Linux (after pacman install xmake + dotnet-sdk-9.0).
- [ ] M1a Server builds on Linux: `xmake build Server` (or CI linux.yml green) → binary exists.
- [ ] M1b Client builds in CI on our fork with UPDATED deps → Artifacts.zip downloadable via `gh run download`.
- [ ] M2  Server boots + listens locally (log evidence).
- [ ] M3  Mod installed into the real game; game launches on 2.31a; RED4ext log lists CyberpunkMP loaded, no crash.
- [ ] M4  In-game client connects to local server (server log shows session).
- [ ] M5  (multi-session/stretch) Friend's machine connects over network.

## Phases (draft — finalize after research workflow wf_701515d5-327 returns)
0. Prep: pacman install; baseline as-is build attempt (server local + client CI) to get the error baseline before changing anything.
1. Dependency bump per research verdicts (RED4ext.SDK → 2.31-supporting version; decide fork-delta porting).
2. Compile-fix loop (bulk work → opus/sonnet workflow): server locally for fast feedback on shared code;
   client via CI cycles (~10 min each). If CI cycle count explodes, invest in local MSVC-compatible
   compile-checking (clang-cl + xwin or msvc-wine) — decide empirically after first CI run.
3. Runtime stack + install into game (per runtime-stack research), M2/M3 smoke tests on this box.
4. M4 connect test; fix runtime crashes (opus agents on gnarly hook bugs; oracle if 2 fixes fail).
5. /paranoid-review of final diff; report with evidence; postmortem → auto-memory.

## Open questions (research workflow in flight)
- Prior art: usable forks/branches (rpc-optimization, server-hosting, dev)? 
- RED4ext.SDK fork delta: swap to upstream vs rebase; custom APIs used by client?
- Exact runtime component versions for 2.31a + Heroic/Proton setup (launcher viability under Proton).
- What Codeware/ArchiveXL/TweakXL submodules are actually for (packaging only?).

## Risks
- xmake version skew (local 3.0.9 vs CI-pinned 2.9.6) — keep CI as source of truth for client.
- windows-latest runner now ships newer WinSDK → protobuf-cpp break; may need explicit SDK pin in CI.
- Launcher/DLL-injection under Proton may fail → fallback: plain red4ext/plugins install if supported.
- Session token budget (~2.2M/afternoon historical limit): build loop is multi-session; PLAN.md + fork branch
  are the durable state; CI artifacts persist on GitHub.
