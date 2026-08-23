# Curated Profile Avatar Assets

The Aethelgard Android client presents only six server-approved profile avatar identifiers: `trailblazer`, `ember`, `verdant`, `tide`, `moon`, and `sunward`. The backend validates this same catalog and intentionally does **not** accept player-uploaded profile imagery.

Each portrait resource is original commission-style game art generated for this project and is placed under `app/src/main/res/drawable-nodpi/` as `aethelgard_avatar_<id>.png`. The character-setup UI resolves resources by this identifier and retains a colored monogram fallback while a resource is unavailable in a development build.

This limited catalog protects player privacy, prevents unsafe arbitrary image uploads, and keeps the first-release account system compatible with a later moderated avatar expansion.
