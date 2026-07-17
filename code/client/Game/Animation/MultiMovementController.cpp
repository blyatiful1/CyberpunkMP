#include "MultiMovementController.h"

#include "AnimationData.h"
#include "Game/Utils.h"

#include <RED4ext/Scripting/Natives/moveComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/anim/AnimFeature_NPCExploration.hpp>

#include "States/Spawning.h"

Core::RawFunc<316482401, void (*)(Red::AnimationControllerComponent* AnimationController, Red::CName Name, Red::Handle<Red::anim::AnimFeature> Feature)> ApplyFeature;

// MOVEDIAG: the controller vtable below mirrors a 2.2-era RE of the engine's
// idle-controller interface; on 2.31a the engine may dispatch into different
// slots. Log the first call the engine makes into each slot so a frozen-puppet
// session shows exactly which parts of the interface are (not) being driven.
#define MOVEDIAG_SLOT(name)                                                                                            \
    {                                                                                                                  \
        static std::once_flag s_diagFlag;                                                                              \
        std::call_once(s_diagFlag, [] { spdlog::info("[MOVEDIAG] engine first-call: {}", name); });                    \
    }

RED4ext::Memory::PoolAI_Movement* MultiMovementController::GetMemoryPool()
{
    return RED4ext::Memory::PoolAI_Movement::Get();
}

MultiMovementController::~MultiMovementController() = default;

void MultiMovementController::PreTick(float delta)
{
    MOVEDIAG_SLOT("PreTick");
}

void MultiMovementController::Tick(float delta)
{
    MOVEDIAG_SLOT("Tick");

    static uint32_t s_tickCount = 0;
    if (++s_tickCount % 300 == 1)
        spdlog::info("[MOVEDIAG] Tick #{} speed={} target=({}, {}, {})", s_tickCount, m_speed, m_position.X,
                     m_position.Y, m_position.Z);

    States::Base::Update update;
    update.Delta = delta;
    update.Speed = m_speed;

    while (auto transition = m_pState->Process(update))
    {
        m_pState = std::move(transition->State);  // NOLINT(bugprone-unchecked-optional-access)
    }
}

void MultiMovementController::GetDeltaTransform(Red::Vector4& positionDelta, Red::Quaternion& rotationDelta)
{
    MOVEDIAG_SLOT("GetDeltaTransform");
    const auto& rawPosition = m_pComponent->owner->transformComponent->localTransform.Position;
    const auto& rawRotation = m_pComponent->owner->transformComponent->localTransform.Orientation;
    const glm::vec3 pos = Game::ToGlm(rawPosition);
    const auto rot = Game::ToGlm(rawRotation);

    auto angles = eulerAngles(rot);
    angles.z = m_angle - angles.z;

    const Red::Vector4 position{pos.x, pos.y, pos.z, 0.f};

    positionDelta = {m_position.X - position.X, m_position.Y - position.Y, m_position.Z - position.Z, 0.f};
    rotationDelta = Game::ToRed(glm::quat(angles));
}

void MultiMovementController::sub_28(bool& unk)
{
    MOVEDIAG_SLOT("sub_28");
    unk = false;
}

void MultiMovementController::sub_30(Red::Vector4& positionDelta, Red::Quaternion& rotationDelta)
{
    MOVEDIAG_SLOT("sub_30");
    
}

void MultiMovementController::sub_38(Red::Vector4& position, Red::Quaternion& rotation, bool& unk1, bool& unk2)
{
    MOVEDIAG_SLOT("sub_38");
    
}

void MultiMovementController::SendAnimationParameters(float delta, Red::Vector4& position, Red::Quaternion& orientation)
{
    MOVEDIAG_SLOT("SendAnimationParameters");
    AnimationData data;
    data.controller = GetName();

    GetAnimationParameters(data);

    m_animationDriver.SendParameters(data);
}

void MultiMovementController::sub_48(RED4ext::Vector4& somePosition1, RED4ext::Quaternion& someRotation1, RED4ext::Vector4& somePosition2, RED4ext::Quaternion& someRotation2)
{
    MOVEDIAG_SLOT("sub_48");
    
}

void MultiMovementController::sub_50(RED4ext::Vector4& somePosition1, RED4ext::Quaternion& someRotation1, float unk, RED4ext::Vector4& somePosition2, RED4ext::Quaternion& someRotation2)
{
    MOVEDIAG_SLOT("sub_50");
    
}

Red::CName MultiMovementController::GetName() const
{
    return NAME;
}

bool MultiMovementController::sub_60()
{
    MOVEDIAG_SLOT("sub_60");
    return true;
}

void MultiMovementController::sub_68()
{
    MOVEDIAG_SLOT("sub_68");
    
}

void MultiMovementController::sub_70()
{
    MOVEDIAG_SLOT("sub_70");
    
}

void MultiMovementController::Mount(RED4ext::move::Component& movable, Red::Handle<Red::ent::Entity> owner)
{
    MOVEDIAG_SLOT("Mount");
    
}

void MultiMovementController::Attach(Red::move::Component& movable)
{
    MOVEDIAG_SLOT("Attach");
    m_pComponent = &movable;

    const auto& pos = movable.owner->transformComponent->localTransform.Position;
    const auto& rot = movable.owner->transformComponent->localTransform.Orientation;

    const auto quat = Game::ToGlm(rot);
    SetTransform(Red::Vector4{pos.x, pos.y, pos.z, 0.f}, eulerAngles(quat).z, 0.f);

    m_animationDriver.Attach(movable.owner);

    m_pState = MakeUnique<States::Spawning>(*this);
    m_pState->Enter();
}

void MultiMovementController::Detach(Red::move::Component& movable)
{
    MOVEDIAG_SLOT("Detach");
    m_animationDriver.Detach();

    m_pState.reset();

    m_pComponent = nullptr;
}

void MultiMovementController::GetAnimationParameters(AnimationData& animationData)
{
    MOVEDIAG_SLOT("GetAnimationParameters");
    m_pState->GetAnimationData(animationData);
}

void MultiMovementController::SetTransform(const Red::Vector4& aPosition, float aAngle, float speed)
{
    m_position = aPosition;
    m_angle = aAngle;
    m_speed = speed;
}

float MultiMovementController::GetAnimLength(Red::CName aName) const
{
    return m_animationDriver.GetAnimLength(aName);
}

void MultiMovementController::Reset()
{
    // representation@0x138 is a fork-RE native member that upstream reflection cannot
    // corroborate (offset-audit: UNVERIFIABLE-STATIC). Sanity-check the block before
    // writing through it: if the offset drifted on this game build these fields will
    // not self-reference, and skipping the reset only stalls animation state, while a
    // blind write would corrupt unrelated puppet memory.
    auto& rep = m_pComponent->representation;
    const bool cRepresentationSane = rep.parent == m_pComponent && rep.activeIndex >= -1 &&
                                     rep.activeIndex <= static_cast<int32_t>(rep.stack.size) &&
                                     rep.stack.size <= rep.stack.capacity &&
                                     (rep.stack.size == 0 || rep.stack.entries != nullptr);
    if (!cRepresentationSane)
    {
        spdlog::error("move::Component.representation@0x138 failed sanity check (parent={}, component={}, "
                      "activeIndex={}, stack={}/{}) — offset drift on this game build; skipping reset",
                      fmt::ptr(rep.parent), fmt::ptr(m_pComponent), rep.activeIndex, rep.stack.size,
                      rep.stack.capacity);
        return;
    }
    rep.stack.Clear();
    rep.activeIndex = -1;
    rep.activeEntry = nullptr;
    rep.active = false;
}
