plugins {
    id("com.android.asset-pack")
}

assetPack {
    packName.set("assetpack_core")
    dynamicDelivery {
        deliveryType.set("install-time")
    }
}
