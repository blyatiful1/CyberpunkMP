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
- [x] M1b Client builds in fork CI with ported SDK — 2026-07-14 run 29355615364 (commit 656e743) GREEN;
        Artifacts.zip (2.6MB, 32 files) downloaded + inspected: mod/CyberpunkMP.dll 4.2MB +
        CyberpunkMP.archive + Inputs + 20 .reds files, launcher-free layout. CI error classes fixed
        along the way: entEntity renames (14 sites), MakeUnique ADL vs new SDK overloads (18 sites),
        IPlacedComponent incomplete type (PCH), SDK AddressResolverOverride clobber, D3D12MemAlloc
        includedir, xmake single-target CLI grammar.
- [x] M2  Server boots + listens locally — 2026-07-14: sustained ≥20s, ss shows udp *:11778 (game) +
        tcp *:11778 (WebApi admin) + tcp 127.0.0.1:27750 (flecs REST); log ~/cp2077-audit/session-state/
        server-boot-m2.log. LAUNCH ENV REQUIRED: DOTNET_ROLL_FORWARD=LatestMajor +
        CYBERPUNKMP_ADMIN_USERNAME + CYBERPUNKMP_ADMIN_PASSWORD (WebApi.cs:108 throws SecurityException
        without them → PAL_SEHException kills process on first Update tick).
- [x] M3  2026-07-15 ~23:00 (visual confirm by user pending): game launches on 2.31a with full stack,
        ZERO validation errors (was 9+), mod initializes, process stable 7min, one clean `-online` run.
        ROOT CAUSE was fork red-lib's IsTypeNameConst rejecting nameof::cstring → classes registered
        under their SDK base's engine name (gameIGameSystem/redEvent), corrupting engine RTTI. Fixed in
        Resolving.hpp (upstream hunks); proven by oracle + in-game probe + binary strings + static_asserts.
- [~] M4  Server+client both run with -online args wired; connect = the UIConnectToServer INPUT ACTION
        (the HUD icon is the vanilla phone hotkey widget — NOT clickable). 2026-07-15 diagnosis of
        "pressing did nothing": -online was active (no "disabled" warning in red4ext log), server was
        up, controller alive (icon spawned), and every GNS failure path logs via OnDisconnected — yet
        CyberpunkMP.log shows ZERO activity after OnInitialize ⇒ the key event never reached OnAction.
        The reds layer was fully blind (no CET ⇒ FTLog lands nowhere). Fix package deployed (zero-CI,
        reds+XML only, scc gate green, verified in output blob): F9→F7 fallback binding (F9 = vanilla
        QuickLoad — a press would have QUICKLOADED), on-screen SimpleScreenMessage diagnostics
        (ready-msg at icon spawn, every UIConnectToServer event type, connect attempt), Jump/Space
        probe to discriminate listener-plumbing vs binding failure, connect on PRESS or HOLD_COMPLETE
        (debounce re-armed by RELEASE). Adversarial verify workflow: wf_1ea71f8d-e62.
        VERIFY BLOCKER FOUND (completeness critic): the mod ran DISABLED in EVERY session so far —
        Heroic's single-dash "-online" parses to game-param key "online"; the check needs "-online",
        i.e. the cmdline needs DOUBLE-dash. Proof: red4ext/logs/cyberpunkmp-2026-07-14-*.log all
        contain 'CyberpunkMP is disabled'. (M3's "clean -online run" = game stable, but network
        paths were never actually exercised; yesterday's key presses are unexplained-but-moot —
        Connect() would have been a silent no-op regardless.)
        FIXED + VALIDATED UNATTENDED 2026-07-15 ~17:00 (two headless launches, game killed after):
        launcherArgs now "--launcher-skip --online --ip=127.0.0.1 --port=11778" (--launcher-skip
        REQUIRED: without it REDlauncher GUI opens and waits for a Play click). Evidence:
        /proc cmdline "Cyberpunk2077.exe --online --ip=127.0.0.1 --port=11778"; NO disabled warning;
        mod log shows enabled-only lines (AppearanceSystem OnInitialize, NetworkWorldSystem/
        InterpolationSystem OnWorldAttached); scc in-game compile green (25894 refs) WITH the
        instrumented reds; input merge landed IK_F7+IK_Space; D3D12 Present hook + enabled update
        pump stable 13+ min at menu. Remaining for M4: user loads a save, presses connect key,
        watches on-screen messages + logs (protocol below).
- [x] M4  DONE 2026-07-15 — user confirmed in-game connect works ("seems to be working").
        Post-confirm cleanup DEPLOYED (takes effect on next game launch, gate green, kit rebuilt):
        German-QWERTZ keybinds — connect AND disconnect both on F7 + <> + # (state machine picks the
        live action; F6/F7/F8 are the ONLY vanilla-free keys, T=11 uses, Enter=13); chat moved
        IK_Semicolon (umlaut key on DE) → F6; TEMP diagnostics removed (IK_Space binding, Jump probe,
        event-type spam, init beacon, debounce-suppressed msg); KEPT: ready message, "connecting...",
        CONNECTED/disconnected messages, audio cues, debounce + both resets.
- [ ] M5  Friend connects remotely (ACTIVE — everything prepared 2026-07-15, two human steps remain):
        Route = Tailscale (daemon active, LOGGED OUT). HOST steps: (1) `sudo tailscale up` + browser
        auth; (2) `sudo ufw allow in on tailscale0` (ufw IS enabled: /etc/ufw/ufw.conf ENABLED=yes —
        without this the friend's UDP 11778 is dropped); (3) invite friend to tailnet, send them
        ~/cp2077-audit/friend-kit/CyberpunkMP-FriendKit.zip (rebuilt sha256 787660ad..., 101 files,
        current assets + fixed README) + host IP from `tailscale ip -4`; (4) recommend real
        CYBERPUNKMP_ADMIN_PASSWORD env before start-server.sh (WebApi tcp 11778 rides the same port;
        default creds admin/localtest123). FRIEND steps: in README-INSTALL.md (game 2.31a, extract
        game-root overlay, Tailscale join, args `--launcher-skip --online --ip=<HOST> --port=11778` —
        README previously taught the BROKEN single-dash form, now fixed). Server config OK for this:
        MaxPlayer 4, Public false; leave server.json Password EMPTY — client hardcodes auth token
        "test" (NetworkService::OnConnected), a server password would reject everyone.
        2026-07-16 evening: friend DID join (Tailscale direct connection, kit install worked) but the
        HOST client CTD'd BOTH times a remote player's puppet spawned while that player was MOVING
        (dumps 20260716-214427 + -214933 in wine prefix ReportQueue; GameThread; REDEngine dumps are
        stripped — no exception stream, use per-thread RIPs + timing instead).
        ROOT CAUSE FOUND: InterpolationSystem::HandleNotifyEntityMove deref'd
        entity.get_mut<InterpolationComponent>() without a null check. flecs v4 (upstream bumped to
        4.0.3 in e7e638f, Nov 2024, their final weeks — latent upstream bug) returns nullptr for
        absent components (v3 auto-added). Window: between NetworkWorldSystem::Spawn() (flecs entity
        exists with only SpawningComponent) and engine spawn completion (EntityComponent+
        InterpolationComponent added by observer). A NotifyEntityMove in that window = null deref.
        Asymmetry evidence: friend survived spawning host's puppet because host stood STILL typing
        chat; host died both times because friend was moving (vehicle entry / on foot).
        FIX: commit 69c30b4 on patch-2.31a (7-line guard: drop move packets while spawning) — all
        other 8 flecs accessor sites in client audited, properly guarded. CI run 29531327449.
        FIX VALIDATED UNATTENDED 2026-07-16 ~22:25: new DLL (CI 29531327449, sha 01900af7...)
        deployed via refresh-mod.sh + working-tree assets re-deployed on top; headless launch with
        temp Diag reds (auto-continue via SingleplayerMenuGameController.OnInitialize wrap +
        PlayerPuppet.OnGameAttached trigger — @wrapMethod on the MOD's own Ink classes fails with
        UNRESOLVED_REF, wrap vanilla classes; chat.Send() as crash-surviving telemetry, server logs
        it). Result: connect + MALE muppet + FEMALE muppet all survived ("DIAG step3 ... PASSED" in
        server log), 0 new dumps, 0 access violations in 37MB WINEDEBUG=+seh capture → the 2.2-era
        archive .ents are FINE on 2.31a; the null deref was the whole story. Diag removed from
        deployed+gate after. KIT REBUILT sha 6c05d741... (101 files, fixed DLL byte-identical to
        deployed, chat rework in, README update-note added). Friend re-extracts kit (Step 2 only).
        KIT REBUILT AGAIN 2026-07-17 ~21:50 sha 9491b4d7... (102 files, DLL = 28922043 = deployed
        M6 movement-fix build 93d7e75; old kit still had pre-fix 01900af7 ⇒ friend would have kept
        the frozen-puppet client half). Tailscale NOW LOGGED IN; friend's machine "lauchburrito"
        (windows) already on tailnet; host IP cachedu = 100.64.176.103. ⚠ DO NOT restart the server
        while playing on this kit: on-disk Server.Loader (rebuilt 21:42) is already the 2d1ea48
        equipment-sync build, but the RUNNING server (pid 40156, started 19:11) is the matching
        93d7e75 build — a restart mixes protocol versions vs both deployed clients + kit. Server
        runs with DEFAULT admin creds admin/localtest123 (tailnet-only exposure, accepted for now).
        Known wart (guard caught it, non-fatal): connecting seconds after load can upload an EMPTY
        ccstate ("CCS state handle drift: instance=0x0" + "CustomizationState was null" in mod log)
        — own CCS state not yet populated; puppet spawns default-looking. Polish backlog (C++ pass):
        remote nameplate hardcoded "Test" (AppearanceSystem.cpp:181), RTTIPROBE init spam removal,
        emote wheel placeholder names (EmoteSelector.reds anim list).
        UX 2026-07-16: ChatController reworked (uncommitted, deployed, gate-green, in kit): panel
        hidden unless connected (UIMultiplayerConnectedToServer BB listener), fake "Connected to..."
        init message removed, auto-fade after 10s idle (wake on message/typing, generation counter +
        anim-stop race guard), F6 hint only while connected. Mouse-escapes-game fix: wine prefix
        registry GrabFullscreen=Y (X11 Driver) — canonical XWayland fullscreen-grab fix, user-verify
        pending next session.
        BUG #3 (SERVER, 2026-07-16 ~22:45): server process DIED via NullReferenceException on the GC
        FINALIZER thread: ~DeliveryDriver() → CancelJob() → PlayerSystem.GetById(disconnected id)
        returns Player wrapping a NULL native IPlayer (loader PlayerManager.cs:30 never returns null
        — Taxi.cs even has dead null-checks expecting otherwise) → Player.SendChat NRE → process
        exit. Trigger: player takes a delivery job, disconnects, GC finalizes. ~Taxi() had the same
        pattern. FIX (uncommitted, built, deployed to all 4 release-tree DLL copies, server
        restarted): Player.SendChat/Id/PuppetId/ConnectionId null-guarded + IsValid added;
        try/catch around both finalizers' CancelJob. Rebuild recipe: DOTNET_ROLL_FORWARD=LatestMajor
        dotnet build -c Release code/server/scripting/JobSystem/JobSystem.csproj → copy
        bin/Release/net8.0/{JobSystem,CyberpunkSdk}.dll over every same-named DLL under
        build/linux/x86_64/release/.
        OBSERVER INSTANCE (2026-07-16, for friend-free 2-client testing): reflink clones (btrfs,
        0 bytes) at ~/Games/observer/{Cyberpunk 2077,prefix}; UserSettings stripped to 720p windowed
        low no-RT; Diag/ObserverAutoConnect.reds ONLY in the observer copy (auto-continue + auto
        connect + "OBSERVER: online" chat beacon; gate-green). Launch: ~/CyberpunkMP/
        launch-observer.sh; stop: WINEPREFIX=/home/crocco/Games/observer/prefix wineserver -k.
        Both processes are "Cyberpunk2077.exe" — distinguish via /proc/<pid>/cwd. Observer logs/
        dumps scrubbed post-clone (its ReportQueue starts empty).
        TWO-INSTANCE RUN 2026-07-17 (user: "concentrate on getting it to run on my pc"):
        v1 attempt FAILED — both clients load the SAME save, so each spawned the other's muppet
        exactly inside its own V; ~23s (main) / ~170ms (obs) after muppet spawn each client's world
        spontaneously reloaded (detach→attach 87ms apart = a save load starting; trigger never
        beacon-confirmed, prime suspect death-by-overlap), and the observer CTD'd 6s into its
        reload on a redDispatcher thread (dump 20260717-174257, stripped; parser:
        session scratchpad parse_dump.py — per-thread RIP+module from streams 3/24/4) with an
        appearance-apply ("Scheduling change") in flight 150ms before the teardown.
        v2 autopilots FIXED it (gate-green, deployed): (a) connected-guard — PlayerPuppet.
        OnGameAttached wrap skips scheduling when UIGameData.UIMultiplayerConnectedToServer is
        already true (Connect() while connected = Close+reconnect, aborts the live session);
        (b) observer teleports V +8m X before Connect() so the two same-save Vs never overlap;
        (c) chat-beacon telemetry on DeathMenuGameController/SingleplayerMenuGameController
        OnInitialize (server logs all chat → post-hoc trigger id). VERIFIED PASS 18:00-18:06:
        both authorized, both beacons relayed, mutual "Logging Entity Appearance", 3-min window
        with 0 world detaches, 0 new dumps (2/1 baselines), all processes alive. NO diag beacon
        fired — muppet-fires-PlayerPuppet-wrap hypothesis unconfirmed; teleport was the load-
        bearing fix. Note: Client::Send with no session is safe (invalid GNS handle, no-op).
        CURRENT STATE: main install autopilot REMOVED (normal play restored; redeploy from
        ~/cp2077-runtime-staging/redscript-gate/r6dir/scripts/Diag/MainAutoConnect.reds for
        future unattended tests); observer keeps v2 autopilot; server pid 5883 running.
        Two-player local workflow: launch main via Heroic, F7 to connect; run launch-observer.sh
        for the second player (fully autonomous, ~2.5 min to in-game).
- [~] M6  FULL MULTIPLAYER PASS (user 2026-07-17 evening: "characters load but dont move; make it
        completely multiplayer passable, animations and connections"). FROZEN-PUPPET ROOT CAUSE
        FOUND + FIXED (commit 93d7e75): Server::GetTick() returned high_resolution_clock
        time_since_epoch() ms — Linux: system_clock ⇒ ~1.75e12; client InterpolateEntity cast
        tick time to FLOAT ⇒ 24-bit mantissa snaps epoch-scale ms to ~131 s steps ⇒
        `tick > future.Tick` false for minutes ⇒ timepoint queue never pops ⇒ every remote
        puppet pinned to first received position. Windows servers (upstream) use QPC-since-boot
        (small) — bug is Linux-host-only, explains why upstream demos worked. Fix: server ticks
        now server-start-relative + client tick math float→double. Diagnosed via MOVEDIAG
        instrumented DLL (commit e8ab59c, CI 29597334997): probes at send/recv/apply/attach +
        first-call log per controller vtable slot + OBSPOS chat-beacon ground truth from a v4
        observer autopilot teleport-patrol (hops arrived+queued while SetTransform applied a
        constant for minutes = the pin). RULED OUT along the way: flecs v4 semantics (opus audit:
        all call sites correct), vtable slot drift (engine first-calls Attach/PreTick/Tick/
        GetDeltaTransform/sub_28/sub_30/SendAnimationParameters/GetAnimationParameters), RawFunc
        hash resolution (all 10 movement-path hashes present in 2.31a addresses DB), attach race
        (controller stored on SpawningComponent, carried over on promote), moveComponent@0x420
        speed gate (never tripped), localTransform-vs-world space (localTransform DOES track:
        OBSPOS ground truth matched sent values 8.01↔10.51 during patrol).
        Analysis artifacts: workflow wf_9bfa0a7f-df4 (flecs audit + movement pipeline map w/ full
        offset/hash inventory + ConnectionManager.reds UX draft at ~/cp2077-audit/drafts/, gate
        NOT yet run on it). Server rebuilt+restarted with fix (xmake Server.Loader; C# fix DLLs
        confirmed intact in rebuild — finalizer string present). VERIFIED 2026-07-17 19:22 (CI
        93d7e75 DLL sha 28922043, both installs): patrol-window logs show SetTransform targets
        HOPPING 8.01↔10.51 every ~3s AND GetDeltaTransform cur FOLLOWING each hop (engine
        physically displaces the puppet) — movement replication WORKS; 0 detaches, 0 new dumps.
        M6 CLOSED by user live-test 2026-07-17 ~21:20: "walking and sprinting animations run" —
        movement + locomotion animations WORK across sessions. User-observed gaps → M7 backlog:
        remote puppet missing CLOTHES + GUNS (equipment sync — the "ItemID creation failed"
        warnings in AppearanceSystem equipment apply are exactly this path), plus the known
        default-appearance-on-fast-connect wart.
        Still queued from M6: ConnectionManager.reds review+deploy (gate-green, unreviewed, at
        ~/cp2077-audit/drafts/); strip MOVEDIAG probes in a cleanup commit; observer autopilot
        stationary v5 deployed.
- [~] M7  SHARED-WORLD CONSISTENCY (user 2026-07-17: "cars and npc are different on the
        sessions; make both sessions run in a singular world, maybe even a new save file").
        Reality check confirmed by recon wf_df3f0151-0ed: CyberpunkMP never synced
        crowds/traffic/NPCs — each client simulates its own world.
        SESSION 2026-07-17 EVENING (state as of ~22:15):
        4→DONE FIRST. EQUIPMENT SYNC ROOT CAUSE: reds GetPlayerItems serialized equipment
           via TDBID.ToStringDEBUG = EMPTY STRINGS without a tweak-name DB (no CET here;
           upstream devs had one). Receiver hashed "" → invalid TDBIDs → "ItemID creation
           failed" ×6. NOT a 2.31a construction drift. FIX (commits 2d1ea48+c6820ed, CI
           29608886383 green, DLL 5c327283 deployed both installs): reds returns raw
           TweakDBIDs (+ drawn WeaponRight item, deduped); wire stays repeated-string but
           carries decimal 40-bit hash+length (offset stripped); receiver parses numeric,
           legacy name-strings still hashed; empty entries skipped. BONUS in same round:
           NotifyCharacterLoad.name field (server sends PlayerComponent.Username; nameplate
           no longer hardcoded "Test", falls back "Player"). PROTOCOL HASH CHANGED (server
           kIdentifier) ⇒ old kits rejected "Invalid protocol version!" — friend must
           re-extract. VERIFIED send-side 22:01: observer log "Getting: 125622758715" ×5
           real decimal IDs (was 6 empty). Receive-side verify pending 2-client session.
        1. SHARED SAVE DONE (adapted): NO save past prologue exists in either prefix — a
           real "new game past prologue" save = hours of play, not automatable. Canonical
           = main AutoSave-8 (V idle at quiet spot LocKey#10964 3 days, on foot, lvl 6,
           Female StreetKid, Act 1 braindance quest). KEY FACT (recon): "Continue" reads a
           save-NAME POINTER in user.gls (SLGR binary, written 0.1s after each save), NOT
           folder mtime ⇒ observer got AutoSave-8's content copied INTO its Continue target
           AutoSave-6 (backup: AutoSave-6.bak-m7); durable copy SharedSave-0 in both
           prefixes + friend kit dir + README section (load via Load Game, NOT Continue).
           sha-verified identical ×4. NOTE: rings re-diverge as each client autosaves —
           re-copy before sessions, or load SharedSave-0 manually.
        2. TIME SYNC IMPLEMENTED (commit e3df3c6, deployed, runtime verify pending):
           NO existing time/weather sync in codebase (recon). Reds-only client via RPC
           auto-discovery (NO proto change): WorldStateClient.SetWorldState in new
           Plugins/WorldState.reds (gate-green; SetWeather 3rd arg is Uint32 → 5u) applies
           TimeSystem.SetGameTimeByHMS + optional weather (empty CName = leave local).
           Server: JobSystem/WorldClock.cs (noon at boot, 8 real s per game min, send on
           join + 60s resync, logs "sent HH:MM:SS to player"). C# stub auto-generated by
           game launch into install Rpc/Client/, copied to JobSystem/Client/. Server
           rebuilt+restarted with it (recipe: dotnet build JobSystem.csproj → cp JobSystem+
           CyberpunkSdk DLLs over all release-tree copies).
        3. CROWD DAMPING DONE: CrowdDensity = the ONLY population lever (no Off; also
           governs ambient traffic; no separate traffic key). Set Medium→Low in BOTH
           UserSettings.json (key appears TWICE per file — /gameplay/performance AND
           /graphics/performance; backups *.bak-m7crowd). True NPC/traffic replication
           scoped honestly by recon: MAJOR new subsystem (new components + protocol +
           population-system hooks + bandwidth for ~100s of transient actors) — NOT
           promised; density parity is the shipped answer.
        OTHER SESSION EVENTS: refresh-mod.sh clean-swap DELETED the running main game's
        open CyberpunkMP.log (user had launched via Heroic 21:50:30 and was playing on
        yesterday's DLL; read via /proc/<pid>/fd — they were connected to the OLD server,
        my restart kicked them 22:03; old client now cleanly rejected by protocol check).
        User push-notified to restart game + F7. MOVEDIAG+RTTIPROBE strip commit 9f5ffa7
        (kept: double tick math, spawn null-guard, +0x50 owner read + drift tripwire,
        controller carry on SpawningComponent). ConnectionManager.reds review verdict:
        FIX-FIRST, 1 blocker (unguarded Connect() in PlayerPuppet.OnGameAttached wrap =
        reintroduces session-abort; also fires for remote muppets) + reconnect-state
        won't survive save-load (plain fields) ⇒ NOT deployed, draft stays in
        ~/cp2077-audit/drafts/. Friend kit README updated (protocol warning + SharedSave
        + crowd Low); kit ZIP rebuild pending final DLL from CI (commits e3df3c6+9f5ffa7).
        Known wart still open: observer uploaded EMPTY ccstate again at 22:01 connect
        ("CCS state handle drift: instance=0x0") 93s after world attach ⇒ NOT purely a
        fast-connect race; remote side shows default V. Reconnect re-uploads.
        LIVE-VERIFY STATE 22:30 (user at PC playing): user restarted onto new DLL and
        connected 22:19:34 with REAL equipment IDs ("Getting: 69264273377" x4, later x6
        after re-equip) + full ccstate 9694B. WorldClock VERIFIED server-side:
        "[Plugin:WorldClock] sent 13:27:04 to player 208" on join + 60s resyncs; clock
        math exact (7.5x from noon@22:07:57 boot). Minor NEW bug: "Failed to retrieve
        Rpc with id D"+"Rpc failed" on FAST RECONNECT only (clean joins fine) =
        definitions-vs-first-RPC race; 60s resync self-heals; fix someday in
        RpcService ordering. RECEIVE-SIDE (clothes visible on remote) STILL UNVERIFIED:
        every observer relaunch tonight was closed BY THE USER (clean "Shutting down"
        in wine log, 0 dumps, timing matches their window-closing while they restarted
        their own game) — observer runs only when user tolerates it; verify next
        two-client session. MainAutoConnect.reds REMOVED from main install 22:26 (user's
        22:23 session still has it compiled in = menu hijack until their NEXT restart —
        it auto-loads last checkpoint + auto-connects, explains their 22:19-22:24
        connect cycling; a muppet/save-reload re-fires it through the addField
        once-guard being per-PlayerPuppet-instance).
        KIT REBUILT sha c5975ac1... 96 files: final DLL ba684006 (9f5ffa7 strip+timesync,
        CI 29610262162 green), SharedSave-0 included, WorldState.reds in, no Diag leak,
        README has protocol-update warning + SharedSave Step 2.5 + crowd-Low note.
        assemble-kit.sh now bundles $KIT_DIR/SharedSave-0 and accepts a custom plugin
        dir (kit built from scratchpad kit-plugin, NOT the live install → autopilot
        can never leak into a kit again). Installs still run c6820ed DLL (identical
        protocol/logic to 9f5ffa7; swap in final DLL at next quiet moment via
        refresh-mod.sh 29610262162 — but ONLY when the user's game is closed:
        refresh-mod clean-swap deletes a RUNNING game's open log (recover via
        /proc/<pid>/fd) and the game keeps the old DLL image anyway).
        Prompt for kickoff saved at ~/cp2077-audit/M7-KICKOFF-PROMPT.txt.

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

## M3 state 2026-07-14 evening — runtime installed, one blocker
- Runtime stack + mod INSTALLED in the real game (hash-verified, zero collisions; Heroic env +
  native d3dcompiler_47 wired; config backup: 1423049311.json.bak-cyberpunkmp).
- Launch result: RED4ext + all 5 plugins load on 2.31a under wine 11.12; scc compiles (25890 refs);
  render hooks survive. BLOCKER: RED4ext ValidateScripts — 9 errors, game exits before menu.
- EXPERIMENTALLY ISOLATED (2 user launches): stack w/o our plugin = CLEAN; our DLL with ZERO .reds
  = 9 hierarchy errors (Event family 3, IGameSystem family 6). ⇒ our DLL's RTTI registrations alone
  corrupt the Event/IGameSystem hierarchy. Mechanism hypothesis: client's bundled 2024 red-lib
  resolves parent classes by SHORT alias name ('Event', 'IGameSystem'); 2.31 real classes are
  redEvent/gameIGameSystem (short names = script aliases); old red-lib auto-creates bogus parents.
  Codeware 1.20.3 (current red-lib) on same install = fine, so upstream has the fix.
  Root-cause workflow: wf_a598104d-d2e → ~/cp2077-audit/m3-experiment/rtti-diagnosis.md.
- Fix loop cost: client DLL changes need a windows CI round (~5.5 min now) + reinstall + user launch.
- start-server.sh added (env recipe baked in). CI: ALL GREEN both platforms as of c6c158e.

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

## M4 test protocol (2026-07-15, user present — every layer below pre-validated except the key press)
1. Server: running fresh (started 16:32, `pgrep -af Server.Loader`; restart: ~/CyberpunkMP/start-server.sh).
   Watch: `tail -f ~/CyberpunkMP/build/linux/x86_64/release/logs/Server.log`. Success marker:
   "Authorize connection from testuser with token test" — NOTE info lines can lag up to 60s
   (flush_on(warn) only); client-side log flushes instantly, trust it first.
   ("could not reach the server list" spam = harmless master-server announce.)
2. Launch game via Heroic as usual (config now has --launcher-skip --online --ip=127.0.0.1 --port=11778;
   RED launcher GUI no longer appears). Load a save (on foot).
3. Expect on-screen (each with a phone-audio cue): "CyberpunkMP controller initialized" at HUD load,
   then "CyberpunkMP ready - on foot, press <>, #, F7 or Space to connect" when the phone icon shows.
4. Press Space once (it's BOTH the Jump probe and — temporarily — a connect key):
   expect "input listener OK (Jump)" and/or "connect key event (type N)" + "connecting to server..."
   (type decode: 0=PRESSED 1=RELEASED 3=HOLD_COMPLETE). Then "CyberpunkMP: CONNECTED to server" or
   "disconnected/failed (reason N)" on screen; CyberpunkMP.log gets "Connected to server."; server log
   gets the authorize line (≤60s lag). F7 / <> / # are the real bindings to try after Space.
5. Decision matrix: no messages at all = controller/UI-channel dead (check CyberpunkMP.log + audio
   cues); Jump-msg only = binding layer dead; connect-msg but no CONNECTED/failed within ~30s = native
   chain (GNS init is the one known silent gap — check CyberpunkMP.log); CONNECTED + no authorize line
   after 60s = server side. Every branch now leaves evidence.
6. On success: remove the TEMPORARY IK_Space button from code/assets/Inputs/CyberpunkMP.xml (+ deployed
   copy) and drop the "connect key event" spam; keep CONNECTED/failed messages.
- Tree state: repo edits (controller+NetworkWorldSystem reds, XML, toggle script, PLAN) UNCOMMITTED;
  deployed copies in game plugin dir are content-identical (verified). Input Loader re-merges XML and
  scc recompiles reds at every launch — reds/XML iteration needs no CI. Gate harness:
  ~/cp2077-runtime-staging/redscript-gate/ (run scc from its tools dir; footguns in
  ~/cp2077-audit/redscript-gate.md). Headless game-launch recipe (validated): kill wine session, then
  gogdl direct with Heroic env — full cmdline in ~/cp2077-audit/validation-launch2-wine.log header /
  this session's transcript; or `heroic --no-gui 'heroic://launch/gog/1423049311'`.
- USER RULE 2026-07-15: workflow agents = opus/sonnet only (explicit model:), Fable-demanding work
  done inline by the orchestrator instead.

## Session/budget notes
- Spent so far ≈ research 295k + plan-critic 84k subagent tokens + orchestrator. Historical cap ~2.2M/afternoon.
- Local gates (clang syntax, redscript CLI) exist to burn seconds instead of 15–40 min CI rounds.
- Durable state: this file, fork branches, CI artifacts, scratchpad/research-results.json.
