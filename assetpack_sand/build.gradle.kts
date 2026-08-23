plugins {
    id("com.android.asset-pack")
}

assetPack {
    packName.set("assetpack_sand")
    dynamicDelivery {
        deliveryType.set("fast-follow")
    }
}
