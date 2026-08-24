package com.darvirgoyt.aethelgrad

import android.app.Activity
import android.content.Intent
import android.content.pm.ActivityInfo
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.view.View
import android.view.Window
import android.view.WindowManager
import android.widget.ImageView

/** Shows the supplied Aethelgrad launch artwork before the sign-in-first game shell. */
class LaunchActivity : Activity() {
    private val handler = Handler(Looper.getMainLooper())
    private val openGame = Runnable {
        startActivity(Intent(this, MainActivity::class.java))
        finish()
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        requestedOrientation = ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE
        requestWindowFeature(Window.FEATURE_NO_TITLE)
        window.setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN)
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        window.decorView.systemUiVisibility = (
            View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                or View.SYSTEM_UI_FLAG_FULLSCREEN
                or View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                or View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                or View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                or View.SYSTEM_UI_FLAG_LAYOUT_STABLE
        )
        setContentView(ImageView(this).apply {
            setImageResource(R.drawable.aethelgard_launch_art)
            scaleType = ImageView.ScaleType.CENTER_CROP
            contentDescription = "Aethelgrad launch artwork"
            setBackgroundColor(android.graphics.Color.BLACK)
        })
        handler.postDelayed(openGame, 900L)
    }

    override fun onDestroy() {
        handler.removeCallbacks(openGame)
        super.onDestroy()
    }
}
