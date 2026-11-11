# Electricity Lines: behavior and implementation

Describes the exported implementation **0.2.0**. The linked source and metadata define this version; gameplay qualification is a separate requirement.

## Behavior

F8 toggles the presentation of existing electrical connections while walking, including connections to idle machines. The feature runs through existing player update/late-presentation callbacks and controls native link rendering. It does not create links, relay energy or change machine power state. Named Input SDK delivery retains normal focus/typing gates.

## State and lifecycle

The hidden/shown preference and cached link/render objects are session state. Scene changes invalidate rooted references; late presentation reconciles the current display after native updates. Startup checks the current binding surface before installing its callbacks.

## Integration and configuration

No additional feature-mod runtime dependency is declared. Included shared headers are source reuse, not extra DLL dependencies.

The [module metadata](../source/mods/electricity-toggle/mod.json) owns the version and declared integration contract. Native modules start at `MoonLab_ModStart`. Game/Unity integrations target the pinned Steam Windows x64 IL2CPP build.

## Implementation sources

- [mod.c](../source/mods/electricity-toggle/mod.c)

## Verification boundary

Only existing connections can be displayed. Lack of visible nearby links is not proof of a power failure. Electrical behavior is unchanged by this feature; rendered compatibility with other line/UI mods still needs qualification.

`python3 build.py --check` verifies the exported source hashes. It does not execute the mod or qualify the behaviors above. Installation and independent build prerequisites are in the [README](../README.md).
