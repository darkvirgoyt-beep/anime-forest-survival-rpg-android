package com.darvirgoyt.aethelgrad

import android.content.Context
import android.media.AudioAttributes
import android.media.SoundPool
import kotlin.math.max
import kotlin.math.min

class GameAudio(context: Context) {
    private val appContext = context.applicationContext

    data class Settings(
        var master: Float = 1f,
        var music: Float = .75f,
        var effects: Float = 1f,
        var ambience: Float = .8f,
        var voice: Float = 1f,
        var muted: Boolean = false,
    )

    private val preferences = appContext.getSharedPreferences("aethelgard_audio", Context.MODE_PRIVATE)
    private val settings = Settings(
        master = preferences.getFloat("master", 1f),
        music = preferences.getFloat("music", .75f),
        effects = preferences.getFloat("effects", 1f),
        ambience = preferences.getFloat("ambience", .8f),
        voice = preferences.getFloat("voice", 1f),
        muted = preferences.getBoolean("muted", false),
    )
    private val soundPool = SoundPool.Builder()
        .setMaxStreams(16)
        .setAudioAttributes(
            AudioAttributes.Builder()
                .setUsage(AudioAttributes.USAGE_GAME)
                .setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION)
                .build()
        )
        .build()

    private val sounds = mutableMapOf<String, Int>()
    private var musicStream = 0
    private var loadingMusicStream = 0
    private var loadingMusicRequested = false

    init {
        load("music_forest", R.raw.aethelgard_forest_exploration)
        load("music_loading", R.raw.aethelgard_wisteria_loading_ambience)
        load("footsteps", R.raw.sfx_footsteps_forest)
        load("sprint", R.raw.sfx_sprint_loop)
        load("slide", R.raw.sfx_slide)
        load("attack", R.raw.sfx_attack_sword)
        load("bow", R.raw.sfx_bow_release)
        load("gather", R.raw.sfx_gather_resource)
        load("craft", R.raw.sfx_craft_workbench)
        load("animal", R.raw.sfx_animal_companion_call)
        load("boss", R.raw.sfx_boss_roar)
        load("ui", R.raw.sfx_ui_click)
    }

    private fun load(name: String, resourceId: Int) {
        sounds[name] = soundPool.load(appContext, resourceId, 1)
    }

    fun playEffect(name: String, loop: Int = 0, rate: Float = 1f) {
        val id = sounds[name] ?: return
        val volume = effective(settings.effects)
        if (volume <= 0f) return
        soundPool.play(id, volume, volume, 1, loop, rate)
    }

    fun playMusic() {
        val id = sounds["music_forest"] ?: return
        loadingMusicRequested = false
        stopLoadingMusic()
        stopMusic()
        val volume = effective(settings.music)
        if (volume > 0f) musicStream = soundPool.play(id, volume, volume, 1, -1, 1f)
    }

    fun stopMusic() {
        if (musicStream != 0) soundPool.stop(musicStream)
        musicStream = 0
    }

    /** Low-intensity ambient loop used while the real world-readiness tasks are still running. */
    fun playLoadingMusic() {
        val id = sounds["music_loading"] ?: return
        loadingMusicRequested = true
        stopMusic()
        stopLoadingMusic()
        val volume = effective(settings.music) * 0.58f
        if (volume > 0f) loadingMusicStream = soundPool.play(id, volume, volume, 1, -1, 1f)
    }

    fun stopLoadingMusic() {
        if (loadingMusicStream != 0) soundPool.stop(loadingMusicStream)
        loadingMusicStream = 0
    }

    fun setMaster(value: Float) { settings.master = clamp(value); persist() }
    fun setMusic(value: Float) {
        settings.music = clamp(value)
        if (loadingMusicRequested) playLoadingMusic() else playMusic()
        persist()
    }
    fun setEffects(value: Float) { settings.effects = clamp(value); persist() }
    fun setAmbience(value: Float) { settings.ambience = clamp(value); persist() }
    fun setVoice(value: Float) { settings.voice = clamp(value); persist() }
    fun playVoice(name: String, loop: Int = 0, rate: Float = 1f) {
        val id = sounds[name] ?: return
        val volume = effective(settings.voice)
        if (volume <= 0f) return
        soundPool.play(id, volume, volume, 1, loop, rate)
    }
    fun setMuted(value: Boolean) {
        settings.muted = value
        if (value) {
            stopMusic()
            stopLoadingMusic()
        } else if (loadingMusicRequested) {
            playLoadingMusic()
        } else {
            playMusic()
        }
        persist()
    }
    fun getSettings(): Settings = settings.copy()

    fun release() {
        stopMusic()
        stopLoadingMusic()
        soundPool.release()
    }

    private fun effective(category: Float): Float = if (settings.muted) 0f else settings.master * category
    private fun clamp(value: Float): Float = max(0f, min(1f, value))

    private fun persist() {
        preferences.edit()
            .putFloat("master", settings.master)
            .putFloat("music", settings.music)
            .putFloat("effects", settings.effects)
            .putFloat("ambience", settings.ambience)
            .putFloat("voice", settings.voice)
            .putBoolean("muted", settings.muted)
            .apply()
    }
}
