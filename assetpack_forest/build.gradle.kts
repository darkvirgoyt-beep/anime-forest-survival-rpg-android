plugins {
    id("com.android.asset-pack")
}

assetPack {
    packName.set("assetpack_forest")
    dynamicDelivery {
        deliveryType.set("fast-follow")
    }
}
