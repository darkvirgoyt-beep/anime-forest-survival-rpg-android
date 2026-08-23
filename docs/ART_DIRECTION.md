# Aethelgard Art Direction

The user-provided board at [`docs/art_reference/game_visual_reference.png`](art_reference/game_visual_reference.png) is the shared visual reference for future game development. It is a direction reference rather than a request to copy any named game, character, logo, or proprietary asset.

## Visual target

The game should use a bright, stylized anime-fantasy presentation with clean silhouettes, vibrant but controlled colors, soft environmental lighting, and readable gameplay composition. Characters and creatures should use clear shape language, expressive faces, practical outfits, and strong separation from the environment. The rendering target is a polished 3D-inspired look, while the current Android vertical slice remains a lightweight procedural OpenGL prototype.

## World and biome direction

The forest should communicate lived-in farming through green meadows, tilled plots, crops, timber homes, fences, paths, cooking or crafting points, and friendly residents. The sand biome should use warm ochre and terracotta colors, adobe or timber settlement structures, market awnings, wells or cooking props, cacti, and local residents. The snow biome should use cool blue-white lighting, ice cliffs, snow-covered terrain, and predator-only encounters with strong silhouettes and readable health bars.

## Characters and creatures

The player character should remain an original cel-shaded anime-fantasy hero with a practical adventurer outfit, expressive face, readable weapon, and distinct silhouette. Villagers should look helpful and grounded rather than combat-focused. Wildlife should be varied and stylized, including passive forest or settlement animals and dangerous snow predators. The current Frostclaw-style predator concept can guide the snow boss silhouette, but all final in-game assets must remain original or properly licensed.

## Lighting and time of day

The lighting direction should support four repeating phases: **day**, **afternoon**, **evening**, and **night**, with a 900-second (15-minute) total cycle: 360 seconds of day, 270 seconds of afternoon, 180 seconds of evening, and a deliberately short 90-second night. Day is bright and green, afternoon adds warm gold, evening shifts toward orange and violet, and night uses deep blue with controlled highlights, stars, and moonlight. Weather runs independently through clear, rain, and thunderstorm states. Rain adds scalable streaks and puddle ripples; thunderstorms add cooler ambient contrast, timed lightning flashes, and future positional thunder audio. Campfires and lanterns remain warm practical-light anchors so the short night stays readable rather than frustrating. The Android HUD should expose the current phase, day number, and weather state during testing.

## Interface direction

The UI should favor compact dark translucent panels, warm readable labels, clear health and stamina bars, simple inventory slots, and contextual quest text. Important state such as the active biome, current time phase, in-game day number, and snow predator HP should be visible without covering the play area.

## Implementation mapping

| Reference area | Current or planned implementation |
|---|---|
| Graphic style | Procedural OpenGL geometry with flat color planes and cel-shaded silhouettes in the Android slice. |
| Player | Original cel-shaded hero with ink-like contours, hard shadow planes, expressive hair, outfit accents, and sword. |
| Forest | Farming plots, crops, homes, trees, paths, animals, and residents. |
| Sand | Settlement buildings, awnings, cactus, warm palette, and residents. |
| Snow | Ice peaks, snow particles, predator-only encounters, and 100 HP predator health display. |
| Time and weather | 900-second four-phase cycle with 90-second night, changing clear colors, scene tint, sun/moon treatment, stars, rain, lightning, campfire/lantern practical lights, and HUD phase/day/weather labels. |
| Future production path | Authored or licensed 3D models, animation clips, environment assets, and a full mobile-optimized renderer. |

## Asset safety

The reference board may guide color, composition, mood, and broad genre language. It must not be used to reproduce recognizable characters, logos, maps, screenshots, or proprietary assets from another game. New assets should be original, user-supplied, generated for this project, or compatible with the project’s licensing requirements.
