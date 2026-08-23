plugins {
    id("com.android.asset-pack")
}

assetPack {
    packName.set("assetpack_voice")
    dynamicDelivery {
        deliveryType.set("on-demand")
    }
}
