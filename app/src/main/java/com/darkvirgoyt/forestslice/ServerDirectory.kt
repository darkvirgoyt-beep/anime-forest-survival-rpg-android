package com.darkvirgoyt.forestslice

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
        ServerRegion("asia", "Aethelgard Asia", "asia.game.aethelgard.example"),
        ServerRegion("europe", "Aethelgard Europe", "eu.game.aethelgard.example"),
        ServerRegion("north_america", "Aethelgard North America", "na.game.aethelgard.example")
    )
}
