plugins {
    id("com.android.asset-pack")
}

assetPack {
    packName.set("assetpack_animation_sets")
    dynamicDelivery {
        deliveryType.set("fast-follow")
    }
}
