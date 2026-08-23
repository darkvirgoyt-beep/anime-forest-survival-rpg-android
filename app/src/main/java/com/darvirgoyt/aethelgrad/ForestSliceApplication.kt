package com.darvirgoyt.aethelgrad

import android.app.Application
import com.google.android.gms.games.PlayGamesSdk

class ForestSliceApplication : Application() {
    override fun onCreate() {
        super.onCreate()
        PlayGamesSdk.initialize(this)
    }
}
