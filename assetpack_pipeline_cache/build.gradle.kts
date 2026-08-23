plugins {
    id("com.android.asset-pack")
}

assetPack {
    packName.set("assetpack_pipeline_cache")
    dynamicDelivery {
        deliveryType.set("fast-follow")
    }
}
