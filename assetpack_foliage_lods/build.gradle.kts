plugins {
    id("com.android.asset-pack")
}

assetPack {
    packName.set("assetpack_foliage_lods")
    dynamicDelivery {
        deliveryType.set("fast-follow")
    }
}
