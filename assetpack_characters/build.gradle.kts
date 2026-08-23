plugins {
    id("com.android.asset-pack")
}

assetPack {
    packName.set("assetpack_characters")
    dynamicDelivery {
        deliveryType.set("fast-follow")
    }
}
