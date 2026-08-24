package com.darkvirgoyt.forestslice.ui

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.darkvirgoyt.forestslice.AssetPreparationState
import com.darkvirgoyt.forestslice.FallbackReason

private val DeepNight = Color(0xFF06141C)
private val Panel = Color(0xFF0B1D27)
private val Ember = Color(0xFFFFCF67)
private val Mint = Color(0xFFB8E5CE)
private val Muted = Color(0xFFB3C4C9)
private val Warning = Color(0xFFFFB58F)

@Composable
fun AssetPreparationOverlay(
    state: AssetPreparationState,
    onRetry: () -> Unit,
    onEnterWorld: () -> Unit,
) {
    Box(
        modifier = Modifier
            .fillMaxSize()
            .background(DeepNight)
            .semantics { contentDescription = "Aethelgard asset preparation" },
        contentAlignment = Alignment.Center,
    ) {
        Surface(
            modifier = Modifier.width(890.dp),
            shape = RoundedCornerShape(24.dp),
            color = Panel,
            border = androidx.compose.foundation.BorderStroke(2.dp, Color(0xFF8F6E37)),
        ) {
            Column(
                modifier = Modifier.padding(horizontal = 66.dp, vertical = 42.dp),
                horizontalAlignment = Alignment.CenterHorizontally,
            ) {
                Text(text = "✦", color = Ember, fontSize = 52.sp, lineHeight = 52.sp)
                Spacer(Modifier.height(12.dp))
                Text(
                    text = titleFor(state),
                    color = Ember,
                    fontSize = 30.sp,
                    fontWeight = FontWeight.SemiBold,
                    letterSpacing = 1.2.sp,
                    textAlign = TextAlign.Center,
                )
                Spacer(Modifier.height(14.dp))
                Text(
                    text = subtitleFor(state),
                    color = Muted,
                    fontSize = 18.sp,
                    letterSpacing = 1.5.sp,
                    textAlign = TextAlign.Center,
                )
                Spacer(Modifier.height(28.dp))
                Box(Modifier.fillMaxWidth().height(2.dp).background(Color(0xFF102E3A)))
                Spacer(Modifier.height(28.dp))
                Text(
                    text = detailFor(state),
                    color = if (state is AssetPreparationState.StarterPackReady) Warning else Mint,
                    fontSize = 18.sp,
                    lineHeight = 27.sp,
                    textAlign = TextAlign.Center,
                )
                Spacer(Modifier.height(30.dp))
                ActionRow(
                    state = state,
                    onRetry = onRetry,
                    onEnterWorld = onEnterWorld,
                )
            }
        }
    }
}

@Composable
private fun ActionRow(
    state: AssetPreparationState,
    onRetry: () -> Unit,
    onEnterWorld: () -> Unit,
) {
    when (state) {
        is AssetPreparationState.HighEndReady -> Button(
            onClick = onEnterWorld,
            colors = ButtonDefaults.buttonColors(containerColor = Mint, contentColor = DeepNight),
            modifier = Modifier.fillMaxWidth().height(58.dp),
        ) { Text("ENTER WORLD", fontSize = 16.sp, fontWeight = FontWeight.Bold, letterSpacing = 1.sp) }
        is AssetPreparationState.StarterPackReady -> Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.spacedBy(14.dp),
        ) {
            OutlinedButton(
                onClick = onRetry,
                modifier = Modifier.weight(1f).height(58.dp),
                colors = ButtonDefaults.outlinedButtonColors(contentColor = Ember),
            ) { Text("RETRY ASSET PREPARATION", fontSize = 14.sp, fontWeight = FontWeight.Bold) }
            Button(
                onClick = onEnterWorld,
                modifier = Modifier.weight(1f).height(58.dp),
                colors = ButtonDefaults.buttonColors(containerColor = Mint, contentColor = DeepNight),
            ) { Text("ENTER WORLD", fontSize = 16.sp, fontWeight = FontWeight.Bold, letterSpacing = 1.sp) }
        }
        else -> Text(
            text = "PREPARING STARTER PACK…",
            color = Ember,
            fontSize = 15.sp,
            fontWeight = FontWeight.Bold,
            letterSpacing = 1.sp,
        )
    }
}

private fun titleFor(state: AssetPreparationState): String = when (state) {
    AssetPreparationState.Idle,
    AssetPreparationState.CheckingCache,
    is AssetPreparationState.Retrying,
    is AssetPreparationState.Downloading -> "PREPARE HIGH GRAPHICS"
    is AssetPreparationState.HighEndReady -> "HIGH GRAPHICS READY"
    is AssetPreparationState.StarterPackReady -> "STARTER PACK READY"
}

private fun subtitleFor(state: AssetPreparationState): String = when (state) {
    AssetPreparationState.Idle,
    AssetPreparationState.CheckingCache -> "CHECKING LOCAL CONTENT CACHE"
    is AssetPreparationState.Retrying -> "RETRYING PRIVATE CONTENT SERVICE"
    is AssetPreparationState.Downloading -> "DOWNLOADING OPTIONAL HIGH-END CONTENT"
    is AssetPreparationState.HighEndReady -> "OPTIONAL HIGH-END CONTENT IS AVAILABLE"
    is AssetPreparationState.StarterPackReady -> "HIGH-END CONTENT UNAVAILABLE • GAMEPLAY UNLOCKED"
}

private fun detailFor(state: AssetPreparationState): String = when (state) {
    AssetPreparationState.Idle -> "The playable starter pack is available while optional graphics are prepared."
    AssetPreparationState.CheckingCache -> "Checking for a previously downloaded high-end manifest."
    is AssetPreparationState.Retrying -> "Private content service unavailable. Attempt ${state.attempt} of ${state.maxAttempts}."
    is AssetPreparationState.Downloading -> "Downloading optional content. Gameplay is never granted or blocked by this request."
    is AssetPreparationState.HighEndReady -> "High-end graphics will be used when the matching content pack is installed."
    is AssetPreparationState.StarterPackReady -> fallbackMessage(state.reason)
}

private fun fallbackMessage(reason: FallbackReason): String = when (reason) {
    FallbackReason.NoManifestConfigured -> "No private graphics service is configured. Enter the world with the bundled starter pack."
    FallbackReason.PrivateServiceUnavailable -> "The private content manifest is temporarily unavailable (HTTP 503). Enter the world with the bundled starter pack and retry later."
    FallbackReason.Unauthorized -> "The private content service rejected this request. Enter the world with the bundled starter pack and sign in again later."
    FallbackReason.InvalidManifest -> "The private content manifest was invalid. The bundled starter pack is safe to use while it is repaired."
    FallbackReason.NetworkUnavailable -> "A network connection is unavailable. Enter the world with the bundled starter pack."
    FallbackReason.CacheInvalid -> "The cached high-end manifest was incomplete. The bundled starter pack is ready while it is downloaded again."
}
