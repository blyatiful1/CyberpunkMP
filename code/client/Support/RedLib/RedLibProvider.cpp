#include "RedLibProvider.hpp"
#include "Red/TypeInfo/Registrar.hpp"

#include <RED4ext/RTTISystem.hpp>

namespace
{
// --- M3 validation-crash diagnostics probe -----------------------------------
// The game exits pre-menu with 9+ ValidateScripts errors claiming the
// Event/IGameSystem hierarchies are inconsistent, reproducibly caused by this
// DLL's RTTI registrations alone (empirically isolated; two mechanism-level
// fixes were no-ops). This probe logs the actual RTTI state for the affected
// names right after ALL post-register callbacks ran — i.e. as close to the
// validator's view as we can observe — so one launch yields decisive data.
// Logged at error level so it lands in CyberpunkMP.log even with terse configs;
// explicitly flushed because the game exits immediately after validation.

void LogTypeState(RED4ext::CRTTISystem* aRtti, const char* aName)
{
    const RED4ext::CName name(aName);

    if (auto* type = aRtti->GetType(name))
    {
        const auto kind = static_cast<int>(type->GetType());
        const char* parent = "<n/a>";
        if (kind == static_cast<int>(RED4ext::ERTTIType::Class))
        {
            auto* cls = static_cast<RED4ext::CClass*>(type);
            parent = cls->parent ? cls->parent->name.ToString() : "<null>";
        }
        spdlog::error("[RTTIPROBE] type '{}': kind={} parent='{}'", aName, kind, parent);
    }
    else
    {
        spdlog::error("[RTTIPROBE] type '{}': ABSENT", aName);
    }

    auto* byScript = aRtti->GetClassByScriptName(name);
    spdlog::error("[RTTIPROBE]   byScriptName('{}') -> '{}'", aName, byScript ? byScript->name.ToString() : "<null>");

    auto* s2n = aRtti->scriptToNative.Get(name);
    auto* n2s = aRtti->nativeToScript.Get(name);
    spdlog::error("[RTTIPROBE]   scriptToNative='{}' nativeToScript='{}'", s2n ? s2n->ToString() : "<none>",
                  n2s ? n2s->ToString() : "<none>");
}

void DumpRttiValidationProbe()
{
    auto* rtti = RED4ext::CRTTISystem::Get();
    spdlog::error("[RTTIPROBE] ==== RTTI state after all post-register callbacks ====");

    for (const char* name :
         {"Event", "redEvent", "IGameSystem", "gameIGameSystem", "AdvertGlitchEvent", "gameIAttitudeManager",
          "NetworkWorldSystem", "CyberpunkMP.World.NetworkWorldSystem", "ChatMessageUIEvent"})
    {
        LogTypeState(rtti, name);
    }

    if (auto* gi = rtti->GetClass("ScriptGameInstance"))
    {
        bool found = false;
        for (uint32_t i = 0; i < gi->staticFuncs.size; ++i)
        {
            auto* func = gi->staticFuncs[i];
            if (func && func->shortName == RED4ext::CName("GetNetworkWorldSystem"))
            {
                found = true;
                spdlog::error("[RTTIPROBE] GetNetworkWorldSystem: staticFuncs[{}] flags: native={} static={}", i,
                              (bool)func->flags.isNative, (bool)func->flags.isStatic);
            }
        }
        spdlog::error("[RTTIPROBE] ScriptGameInstance staticFuncs={} GetNetworkWorldSystem present={}",
                      gi->staticFuncs.size, found);
    }
    else
    {
        spdlog::error("[RTTIPROBE] ScriptGameInstance class: ABSENT");
    }

    auto* giScript = rtti->GetClassByScriptName("GameInstance");
    spdlog::error("[RTTIPROBE] byScriptName('GameInstance') -> '{}'", giScript ? giScript->name.ToString() : "<null>");

    spdlog::default_logger_raw()->flush();
}
} // namespace

void Support::RedLibProvider::OnBootstrap()
{
    Red::TypeInfoRegistrar::RegisterDiscovered();

    // Queued after RegisterDiscovered so the engine runs it after the
    // framework's own OnDescribe pass — see probe comment above.
    RED4ext::CRTTISystem::Get()->AddPostRegisterCallback(&DumpRttiValidationProbe);
}
