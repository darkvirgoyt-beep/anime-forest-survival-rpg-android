# Aethelgard Game TODO

- [x] Define the original cinematic fantasy login composition and online-only interaction rules
- [x] Replace the current onboarding layout with an original title crest, world vignette, region selector, account-status panel, and settings entry
- [x] Preserve the secure Google Play sign-in, mandatory server readiness, consent gate, and explicit authentication-state handling
- [x] Keep guest mode unavailable in release builds and avoid copying supplied reference artwork, logo, or text
- [x] Validate the redesigned Android APK through public CI and publish the milestone artifact
- [x] Calculate the current Android debug signing SHA-1 and SHA-256 for Google OAuth registration
- [x] Create an original, legally clean Aethelgard cinematic login background asset without using the supplied reference image
- [x] Integrate the original background asset into the Android login experience with a procedural fallback and dark UI-safe vignette
- [x] Validate the packaged background asset in the Android release build and public CI
- [x] Add a standard Google Sign-In backend contract for testing without Play Console, while preserving the future Play Games path
- [x] Implement backend ID-token verification, game-account upsert, and secure rotating Aethelgard sessions
- [x] Replace the Android Play Games-only login attempt with a standard Google Sign-In token exchange flow
- [x] Document the required Android and Web OAuth clients, backend secrets, deployment, and Play Games upgrade path
- [x] Validate the standard Google Sign-In backend and Android release build through automated tests and CI
- [x] Provision a managed HTTPS standard-Google-sign-in development backend with database-backed Aethelgard sessions
- [ ] Publish the managed backend on a stable production HTTPS domain before public release
- [x] Connect the Android login configuration to the managed backend after the Web OAuth client ID is created
