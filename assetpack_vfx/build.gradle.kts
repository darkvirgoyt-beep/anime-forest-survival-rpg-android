plugins {
    id("com.android.asset-pack")
}

assetPack {
    packName.set("assetpack_vfx")
    dynamicDelivery {
        deliveryType.set("on-demand")
    }
}
