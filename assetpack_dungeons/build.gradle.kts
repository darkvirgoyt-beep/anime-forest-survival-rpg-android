plugins {
    id("com.android.asset-pack")
}

assetPack {
    packName.set("assetpack_dungeons")
    dynamicDelivery {
        deliveryType.set("on-demand")
    }
}
