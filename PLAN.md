# CyberpunkMP revival — CP2077 patch 2.31a (GOG/Heroic/Proton)

**Original request (verbatim):** "plan this complete thing out, use workflows of opus 4.8 and sonnet 5 to safe on token usage and build the thing. you build the plan and opus/sonnet build it"

**Goal:** blyatiful1/CyberpunkMP (branch `patch-2.31a`) builds and runs against Cyberpunk 2077 v2.31a
(user's install: `~/Games/Heroic/Cyberpunk 2077`, GOG via Heroic/Proton). Orchestrator = Fable;
builders = Opus 4.8 ('opus') + Sonnet 5 ('sonnet') workflow agents.
Plan reviewed by plan-critic (2026-07-12): PROCEED-WITH-FIXES — all fixes folded in below.

## Load-bearing facts (research wf_701515d5-327 + plan-critic, full JSON: scratchpad/research-results.json)
- **No prior art anywhere.** Upstream dead since 2024-12-21 and doesn't build since mid-2025 (issue #53).
- **Patch 2.31a confirmed current** (2025-09-11; nothing newer as of 2026-07-12).
- **Old SDK would RUN on 2.31a but corrupt silently** (plugin is V0_RUNTIME_INDEPENDENT, addresses resolve
  by hash at runtime; struct layouts changed ⇒ silent memory corruption, not clean errors). So the SDK
  port is mandatory, not optional.
- **SDK port target: WopsS/RED4ext.SDK @ `3ccf163b`** ("Add support for patch 2.31 (#188)", = master~28).
  Master tip would additionally break the client (v0→v1 API refactor: Api/Sdk.hpp deleted,
  CBaseRTTIType→rtti::IType). Verified: Api/Sdk.hpp still exists at 3ccf163b; fork merge-base = 455d9b8b.
- **Port method (per plan-critic SIMPLER): single squash port** — apply `git diff 455d9b8b f52af6a3 -- include/`
  (fork's net change) onto 3ccf163b in ONE conflict pass, minus DROP hunks. NOT 23 sequential cherry-picks
  (same files touched repeatedly; pick-sets not closed under fixups — e.g. dropped 0bd19641 fixes kept 46c826d7).
- **Conflict policy (corrected by critic):**
  - Fork-only headers → carry as-is: gameItemModParams.hpp, gameAddItemToSlotContext.hpp, entPuppet.hpp,
    entComponentsStorage.hpp, entIComponent.hpp, FixedPoint.hpp, Unks.hpp, inkTextWidget/inkTextInputWidget ext.
  - `moveComponent.hpp`: NOT keep-fork — upstream 3ccf163b has its own with 2.31-validated members
    (worldTransform@0x1C0, position@0x1E8, ASSERT_SIZE 0x2C0). MERGE: graft fork's representation@0x138
    (+ stack/activeIndex members client needs, MultiMovementController.cpp:156-159) into upstream's unk blob.
  - `entEntity.hpp`: client's real dependency is componentsStorage.components (Entity.h:35) → reconcile with
    upstream 2.31 regen (raw uint8_t componentsStorage[0x30] + separate components member won't compile as-is).
  - vehicleBaseObject/WheeledBaseObject: keep fork's RE members, reconcile against upstream regen.
  - Keep: MountingInfo/MountingSlotId ctors, ccState virtuals, ISerializable GetType + AddressHashes (46c826d7)
    WITH the corrections from 0bd19641 (ISerializable ctor removal, ASSERT_SIZE fixes) applied.
  - Drop: fa1def5a (CI delete), f52af6a3 (HashMap tip), bc64976e (perf tweaks).
- **Offsets audit BEFORE first game launch** (critic finding 6): rendering hook installs at Bootstrap
  *before* the -online gate ⇒ stale Rendering.h (pads 0xC97F38/0x13BC4D0 + rel32 scan at +0x1F of
  CRenderNode_Present_DoInternal, hash 2468877568) = CTD at every launch. Statically audit against the local
  2.31a exe (`~/Games/Heroic/Cyberpunk 2077/bin/x64/Cyberpunk2077.exe`) + RED4ext v1.30.0 address DB + CET
  as cross-reference. Same for Puppet.h m_moveComponent@0x420 (upstream 2.31 gamePuppet ASSERT_SIZE 0x5F8)
  and move::Component members.
- **Redscript layer must be gated locally** (critic blocker 3): 28 .reds files, 29 @wrapMethod/@replaceMethod
  against 2.2-era vanilla classes. Gate: native-Linux redscript CLI compile against the user's own 2.31a
  `r6/cache` + Codeware v1.20.3 scripts. No phase may skip this.
- **Client compile-check locally without CI** (critic finding 5): xwin (no sudo, static bin) + clang 22
  `--target=x86_64-pc-windows-msvc -fms-compatibility -fsyntax-only` reproduces MSVC record layout ⇒ evaluates
  every RED4EXT_ASSERT_SIZE/OFFSET static assert in seconds. Primary Phase-1 gate; CI stays the final arbiter.
- **M1b artifact: strip Launcher from install/xpack on our branch** (critic finding 8): xpack currently
  builds the 2-year-stale Overwolf-Electron launcher (pnpm) in CI; useless on Linux and a rot hazard.
  Upload Client DLL + assets directly.
- **Server address plumbing already exists** (critic finding 9): client reads `-online -ip <ip> -port <port>`
  game args (Settings.cpp:10-23; defaults 127.0.0.1:11778). M3 smoke test must include one `-online` run
  (network paths gated behind IsDisabled()).
- **Runtime stack for 2.31a** (install layout `red4ext/plugins/zzzCyberpunkMP/`): RED4ext loader v1.30.0
  (bin/x64/winmm.dll), redscript v0.5.31 (NOT 1.0 prereleases), Codeware v1.20.3, ArchiveXL v1.26.8,
  TweakXL v1.11.3, Input Loader v0.2.3. Main.cpp hard-aborts if any missing. Launcher NOT needed on Linux.
- **Proton setup:** Heroic env `WINEDLLOVERRIDES=winmm,version=n,b` (exact); winetricks d3dcompiler_47 only
  (vcrun2022 = prefix-corruption history, last resort); Heroic config ~/.config/heroic/GamesConfig/1423049311.json.
- ~~asset install ".xml.xml" bug~~ — PHANTOM (critic blocker 1): xmake path.basename STRIPS the extension;
  the install rules are correct. Do NOT "fix". Verify real Artifacts.zip layout at M1b instead.
- **Working-tree coordination:** local-server-build workflow leaves uncommitted changes (M xmake.lua,
  ?? xmake_local_packages.lua). Any future commit touching xmake.lua MUST `git add xmake_local_packages.lua`
  with it (critic blocker 2) — an include of an untracked file pushed alone hard-fails CI config.

## Milestones (runnable end-checks)
- [x] M0  Toolchain: user-local xmake 3.0.9 + dotnet 9.0.315 (pacman blocked on sudo password).
- [x] M1a Server builds on Linux — 2026-07-14: `xmake build -y Server.Loader` exit 0 twice (34s full/6s
        incremental); artifacts: Server.Loader apphost + libServer.Native.so 3.8MB + CyberpunkSdk.dll.
- [ ] M1b Client builds in fork CI with ported SDK → artifact downloadable (launcher stripped from xpack).
- [x] M2  Server boots + listens locally — 2026-07-14: sustained ≥20s, ss shows udp *:11778 (game) +
        tcp *:11778 (WebApi admin) + tcp 127.0.0.1:27750 (flecs REST); log ~/cp2077-audit/session-state/
        server-boot-m2.log. LAUNCH ENV REQUIRED: DOTNET_ROLL_FORWARD=LatestMajor +
        CYBERPUNKMP_ADMIN_USERNAME + CYBERPUNKMP_ADMIN_PASSWORD (WebApi.cs:108 throws SecurityException
        without them → PAL_SEHException kills process on first Update tick).
- [ ] M3  Runtime stack + mod installed into real game; launches on 2.31a; red4ext log lists CyberpunkMP;
        menu reachable; one run with `-online` arg. PRECONDITION: static offset audit + redscript gate green.
- [ ] M4  In-game client connects to local server (server log session).
- [ ] M5  (stretch, next sessions) Friend connects remotely: port-forward/WAN or VPN, friend install kit,
        matching game patch. Owns the user's actual goal — must be planned before declaring victory overall.

## Phases
0. ~~Prep + baselines~~ done (fork, branch, toolchain, CI xmake 3.0.9 + vs_sdkver pin dropped).
1. **SDK port** (workflow: sonnet setup → opus port → gate loop):
   a. xwin splat + verified clang MSVC-target syntax-check harness (sonnet).
   b. Squash port onto 3ccf163b per conflict policy above (opus, in vendor/RED4ext.SDK,
      branch `cp2077-2.31a`).
   c. Gate loop: clang -fsyntax-only over SDK headers incl. static asserts; fix until green (sonnet→opus).
   d. Push branch to blyatiful1/RED4ext.SDK; repoint .gitmodules + submodule pointer (commit only those files).
2. **Client/server code + scripts** (workflow): fix client compile errors against ported SDK (clang gate
   locally, CI as arbiter); strip Launcher from xpack/install; redscript compile gate vs user's r6/cache;
   commit xmake.lua + xmake_local_packages.lua together.
3. **Build loop until M1a+M1b green** (workflow): server local, client CI cycles (gh run watch/download).
   In parallel (sonnet/opus): static offset audit of Rendering.h / Puppet.h / moveComponent vs local 2.31a exe.
4. **Runtime install + smoke (M2/M3/M4)** (next session, user present): install runtime stack + built mod,
   Heroic env var + d3dcompiler_47, launch, red4ext log check, `-online` connect to local server.
5. **Review + report:** /paranoid-review of full diff; evidence-backed report; postmortem → auto-memory.

## Results ledger (updated as workflows land)
- **Netpack de-vendored (wf_4753d493-31b, 2026-07-14):** the 3k-line fork of protoc's internal cpp
  helpers (source of ALL protobuf-drift build breaks) replaced by minimal self-contained
  code/netpack/cpp/helpers.h — exactly the 8 naming helpers main.cpp uses, extracted verbatim
  (ClassName/PrimitiveTypeName spot-checked byte-identical vs deleted fork). Emitted names + protocol
  hash preserved; end-to-end generation verified on all 3 protos. NO protobuf version pin needed
  (system 35.1 everywhere; GNS v1.6.0 untouched).
- **M1a+M2 green 2026-07-14** (see milestones). Server self-creates config/ (server.json Port=11778).
  Commit d8e1217 pushed (netpack + D3D12MemAlloc includedir + Launcher strip + scripting dotnet fixes
  + xmake_local_packages.lua).
- **FIXED (wf_b5575164-da5, 2026-07-14): shutdown heap corruption.** TWO compounding defects:
  (1) real invalid-free: World.cpp handed flecs REST `.ipaddr = const_cast<char*>(string.c_str())`;
  flecs OWNS ipaddr and ecs_os_free()s it at ecs_fini → freeing a std::string buffer. Fix:
  ecs_os_strdup (matched allocator pair). (2) ordering: Run() had no signal handling; CoreCLR's
  SIGTERM handler called exit() from a foreign thread mid-progress(). Fix: async-signal-safe
  atomic flag + SIGTERM/SIGINT handlers, loop exits normally, Kill() before World teardown.
  Empirical gate: 2× boot→SIGTERM cycles, exit 0, coredump count unchanged, orderly log.
  (Signal fix alone still aborted — that intermediate run is what isolated the ipaddr bug.)
- **Debug recipe:** managed exceptions crossing reverse-P/Invoke die as opaque PAL_SEHException +
  terminate on .NET 9; rerun with DOTNET_LegacyExceptionHandling=1 to get the real C# stack printed.
- **Windows CI scoped to client chain (2026-07-14):** `xmake -y Client Archives Inputs Tweaks redscript`.
  Reason: Server.Scripting's SdkGenerator (CppSharp 1.1.5, 2024) silently generates NOTHING on 2026
  windows-latest (VS toolchain unsupported; ran 24s, exit 0, no CyberpunkSdk.Internal.cs → CS0234
  cascade). Bindings are gitignored, not committed. Server coverage lives in linux CI (green).
  Revisit only if a windows-hosted server is ever needed (not our path — we host on linux, friend
  needs client only). Fix directions if needed: bump CppSharp / pin VS toolset / print codegen
  stdout via os.iorunv (currently swallowed by os.runv).
- **Offset audit (wf_d17e2fe7-de3, 2026-07-12): ALL Rendering.h offsets CONFIRMED for 2.31a** by disassembly
  of the real exe (0xC97F38 via `lea rsi,[r11+0xc97f38]` in ResizeBackbuffer; stride 0xB0; pDirectQueue@0x13BC4D0
  = ID3D12CommandQueue::Signal; hash 2468877568 → RVA 0x21C5AC via shipped bin/x64/cyberpunk2077_addresses.json;
  +0x1F rel32 verified). gamePuppet 0x5F8 / moveComponent 0x2C0 / speed@0x220 confirmed vs upstream 2.31 asserts.
  UNVERIFIABLE-STATIC (runtime gates required, recipes in ~/cp2077-audit/offset-audit.md):
  m_moveComponent@0x420 (Puppet.h), representation@0x138 (MultiMovementController::Reset).
  → Phase 2 MUST apply: Puppet.h static_asserts + the two runtime sanity gates (bail+log, don't trust-deref).
- **Redscript gate (same wf): GREEN** — all 28 .reds compile against the user's real 2.31a final.redscripts
  + Codeware v1.20.3 via scc.exe v0.5.31 under wine. Zero method-wrap drift. Gate recipe in
  ~/cp2077-audit/redscript-gate.md (gotcha: -compile's SCRIPT_PATH only derives r6_dir; use -compilePathsFile).
- **Runtime stack staged + sha256-verified** (58 files): ~/cp2077-runtime-staging/staged/.
  ⚠ 2026-07-14: MANIFEST.md is GONE (probably lived in /tmp); the 58 files + layout are intact
  (verified). Before the M3 install, re-hash against the upstream release archives in
  ~/cp2077-runtime-staging/downloads/ instead of trusting the missing manifest.
  Layout corrections vs research: Codeware scripts live in red4ext/plugins/Codeware/Scripts (not r6/scripts);
  Input Loader xmls → r6/cache/*.xml + engine/config/platform/pc/input_loader.ini (not r6/inputs).
- **CI baseline (run 29209014800): all third-party packages now build on windows-latest** (xmake 3.0.9,
  no vs_sdkver pin); failure moved into first-party code: netpack helpers.h uses protobuf-removed
  `EffectiveStringCType` (Phase 2 fix: port helpers.h to new protobuf API or pin protobuf-cpp version).

## SHUTDOWN STATE 2026-07-12 (~23:55) — resume here tomorrow
- **Phase 1 (SDK port): COMPLETE.** Submodule vendor/RED4ext.SDK = df23758c ("Port tiltedphoques CyberpunkMP
  fork delta onto upstream 2.31 support") on 3ccf163b, pushed to blyatiful1/RED4ext.SDK@cp2077-2.31a.
  .gitmodules repointed; parent commit a47f6da pushed. Gate: clang-MSVC tu_basic + tu_focus exit 0, all
  ASSERT_SIZE/OFFSET pass (move::Component 0x2C0, WheeledBaseObject 0xBF0, entEntity 0x160 w/ grafted typed
  componentsStorage@0x70). ⚠ Runtime-validation items ("GATE ITEMS", e.g. WheeledBaseObject::engineData@0xBB0
  is a +0x10 rebase ASSUMPTION vs fork's 2.30-era 0xBA0): full list in
  ~/cp2077-audit/session-state/sdk-port-full-result.txt — fold into Phase 4 smoke checks.
  Setup quirk to reuse: clang has no -imsvc (use -isystem); harness copy:
  ~/cp2077-audit/session-state/sdk-check/check.sh (xwin splat persists at ~/.xwin, 641M).
- **Server build loop (wf_4083e5da-984): STOPPED mid-iteration by user request.** It had fixed GNS/OpenSSL
  and was mid-netpack/protobuf fix. Uncommitted working-tree edits (KEEP): code/netpack/NetPackPCH.h,
  code/netpack/main.cpp, code/server/scripting/xmake.lua, xmake.lua, ?? xmake_local_packages.lua.
  Its journal: ~/cp2077-audit/session-state/server-build-journal.jsonl. Resume:
  Workflow({scriptPath: "/home/crocco/.claude/projects/-home-crocco/19d664ac-0ef4-4e0c-bacd-a4e7977cae96/workflows/scripts/cyberpunkmp-local-server-build-wf_4083e5da-984.js", resumeFromRunId: "wf_4083e5da-984"})
  — but CHECK TREE STATE FIRST (completed iterations replay from cache; the killed iteration's partial
  edits are already in the tree — consider a fresh relaunch with a state-describing prompt instead).
- **CI:** parent push a47f6da auto-triggered fork CI runs that will fail in CLIENT code against the new SDK —
  that failure log is the intended Phase-2 error baseline; read it tomorrow before launching Phase 2.
- **Next session order:** (1) re-read this file; (2) check CI baseline errors from a47f6da runs;
  (3) finish server build (resume/relaunch loop); (4) Phase-2 client-fix workflow: netpack protobuf port,
  client-vs-SDK compile fixes (local check.sh gate + CI arbiter), Puppet.h hardening asserts + 2 runtime
  sanity gates (offset-audit recipes), strip Launcher from xpack, commit rules (xmake.lua +
  xmake_local_packages.lua together); (5) M1a/M1b; then Phase 4 smoke (user present for game launches).
- Everything volatile copied out of /tmp: ~/cp2077-audit/session-state/ (research JSON, fork-delta patches,
  sdk-check harness, both workflow journals, full SDK-port result).

## Session/budget notes
- Spent so far ≈ research 295k + plan-critic 84k subagent tokens + orchestrator. Historical cap ~2.2M/afternoon.
- Local gates (clang syntax, redscript CLI) exist to burn seconds instead of 15–40 min CI rounds.
- Durable state: this file, fork branches, CI artifacts, scratchpad/research-results.json.
