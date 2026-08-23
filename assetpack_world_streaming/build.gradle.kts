plugins {
    id("com.android.asset-pack")
}

assetPack {
    packName.set("assetpack_world_streaming")
    dynamicDelivery {
        deliveryType.set("fast-follow")
    }
}
