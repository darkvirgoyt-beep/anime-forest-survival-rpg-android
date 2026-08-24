package com.darkvirgoyt.aethelgrad

data class ServerRegion(
    val id: String,
    val name: String,
    val host: String,
    val pingMs: Int? = null,
    val status: String = "DISCOVERY PENDING",
    val capacity: String = "—"
)

object ServerDirectory {
    val regions = listOf(
        ServerRegion("asia", "Aethelgrad Asia", "asia.game.aethelgard.example"),
        ServerRegion("europe", "Aethelgrad Europe", "eu.game.aethelgard.example"),
        ServerRegion("north_america", "Aethelgrad North America", "na.game.aethelgard.example")
    )
}
