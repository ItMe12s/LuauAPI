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
    # FMOD engine state records.
    "FMODMusic",
    "FMODSound",
    "FMODQueuedEffect",
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
    # Area-effect + song + dynamic-action records (leaf-first).
    "EnterEffectAnimValue",  # leaf: int/float + EasingType enum
    "EnterEffectInstance",  # map<int,EnterEffectAnimValue> + floats + object ptr
    "SongTriggerState",  # object ptr + double
    "DynamicObjectAction",  # object ptrs + floats + ints
    # Editor state.
    "GameObjectEditorState",  # CCPoint + floats
    # Replay records.
    "RecordCheckpoint",  # ints + uint64 + gd::string
    "RecordButtonCommand",  # PlayerButton enum + bools + int
    "PlayerButtonCommand",  # PlayerButton enum + bools + int + double
)
