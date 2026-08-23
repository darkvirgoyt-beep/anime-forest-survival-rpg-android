plugins {
    id("com.android.asset-pack")
}

assetPack {
    packName.set("assetpack_cinematics")
    dynamicDelivery {
        deliveryType.set("on-demand")
    }
}
