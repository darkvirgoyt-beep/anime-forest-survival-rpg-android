plugins {
    id("com.android.asset-pack")
}

assetPack {
    packName.set("assetpack_terrain_lod")
    dynamicDelivery {
        deliveryType.set("fast-follow")
    }
}
