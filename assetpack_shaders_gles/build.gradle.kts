plugins {
    id("com.android.asset-pack")
}

assetPack {
    packName.set("assetpack_shaders_gles")
    dynamicDelivery {
        deliveryType.set("fast-follow")
    }
}
