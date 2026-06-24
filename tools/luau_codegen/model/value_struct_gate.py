from __future__ import annotations

# List of value struct class names to bind. Leaf-first order for nesting.

VALUE_STRUCT_OPT_IN: tuple[str, ...] = (
    # Leaf structs (no value-struct dependencies).
    "ChanceObject",
    "GJPointDouble",
    "SequenceTriggerState",
    "GJTransformState",
    "SavedObjectStateRef",
    "SavedActiveObjectState",
    "SavedSpecialObjectState",
    "TimerItem",
    "GameObjectPhysics",
    "DynamicMoveCalculation",
    "GJValueTween",
    "CAState",
    "FMODSoundTween",
    "SoundStateContainer",
    "FMODQueuedMusic",
    "FMODSoundState",
    # Effect action records (flat primitives + enums + containers).
    "PulseEffectAction",
    "CountTriggerAction",
    "SpawnTriggerAction",
    "OpacityEffectAction",
    "CollisionTriggerAction",
    "ToggleTriggerAction",
    "TouchToggleAction",
    "TimerTriggerAction",
    # Large state blobs (reference the leaves above).
    "GJGameState",
    "GJShaderState",
    "EffectManagerState",
    "FMODAudioState",
)
