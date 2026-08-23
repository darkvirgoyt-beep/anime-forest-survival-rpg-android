plugins {
    id("com.android.asset-pack")
}

assetPack {
    packName.set("assetpack_audio_hd")
    dynamicDelivery {
        deliveryType.set("fast-follow")
    }
}
