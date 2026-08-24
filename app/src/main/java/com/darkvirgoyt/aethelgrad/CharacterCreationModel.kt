package com.darkvirgoyt.aethelgrad

data class CharacterCreationState(
    var name: String = "",
    var eyebrowStyle: Int = 0,
    var outfitStyle: Int = 0,
    var hairStyle: Int = 0
) {
    fun validate(): String? {
        val cleanName = name.trim()
        if (cleanName.length !in 3..16) return "Name must be 3–16 characters"
        if (!cleanName.matches(Regex("[A-Za-z0-9 _-]+"))) return "Use letters, numbers, spaces, _ or -"
        if (eyebrowStyle !in 0..3) return "Choose an eyebrow style"
        if (outfitStyle !in 0..3) return "Choose an outfit"
        if (hairStyle !in 0..3) return "Choose a hairstyle"
        return null
    }
}
