plugins {
    id("com.android.asset-pack")
}

assetPack {
    packName.set("assetpack_graphics_base")
    dynamicDelivery {
        deliveryType.set("on-demand")
    }
}
