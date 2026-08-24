# DeepSeek Harness integration note

## What it is

The official DeepSeek Harness (`dsh`) is an open-source coding-agent harness from DeepSeek AI. Its architecture treats models, tools, skills, sessions, sandboxes, storage, loops, scheduling, and the UI as plugins. It provides Standard, Code, Minimal, and Creator runtime modes, and records runs in an append-only session log that can be resumed, forked, searched, and replayed.

## Relevant official setup

The official quick start is `npx @deepseek-ai/dsh web`, which starts a local web UI. The source setup is `git clone https://github.com/deepseek-ai/deepseek-harness`, followed by `pnpm install`, `pnpm run build`, and `pnpm dsh web`. The project is in developer preview and warns that compatibility-breaking changes may occur.

## Safe role in Aethelgrad

DeepSeek Harness can be used as an optional **developer orchestration layer** for repository inspection, code-generation proposals, test planning, and traceable development sessions. It is not the Unreal Engine itself, not a replacement for the Unreal Editor or UnrealBuildTool, not a cloud-save backend, not a Google Play Games Services provider, and not a multiplayer game server. It cannot turn the current Android prototype into a finished AAA game automatically.

The Aethelgrad source of truth remains the public GitHub repository and the Unreal 5.6+ project. Any Harness-generated changes must pass the same review, source hygiene, host-test, CI, Unreal compilation, device, networking, asset-license, and release gates. Secrets such as Google credentials, Play signing keys, server tokens, and backend credentials must not be placed in prompts, repository files, or APK assets.

## Suggested Aethelgrad Harness workflow

1. Mount or clone the Aethelgrad repository in a local Harness sandbox.
2. Load the `aaa-android-survival-rpg` skill and the current milestone checklist.
3. Ask for a plan-only change first, with explicit files, acceptance tests, and Unreal-version assumptions.
4. Ask for one bounded slice, such as an account/server-selection data contract or a character-customization schema.
5. Run source validation and host tests before reviewing the diff.
6. Compile the Unreal module on a machine with Unreal Engine 5.6+ installed.
7. Build the Android milestone through CI and test on physical devices.
8. Merge only reviewed, validated commits.

## Initial build order requested by the user

The first Aethelgrad product structure should be: boot and account boundary, Google Play sign-in with offline guest mode, server/region selection showing latency and status, character creation with original appearance controls and name validation, world creation or co-op world join, cloud-save and conflict policy, first forest spawn, cave entrance, mission tracker, navigation markers, gathering/crafting, creature interaction and bonding, and eventual four-player co-op authority. Each item is a milestone, not a single generated operation.

## Official sources

- https://deepseek.com/harness/en/
- https://github.com/deepseek-ai/deepseek-harness
- https://deepseek-harness.github.io/deepseek-harness/en/guide/quickstart
