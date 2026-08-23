plugins {
    id("com.android.asset-pack")
}

assetPack {
    packName.set("assetpack_snow")
    dynamicDelivery {
        deliveryType.set("fast-follow")
    }
}
