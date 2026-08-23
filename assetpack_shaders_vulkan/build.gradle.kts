plugins {
    id("com.android.asset-pack")
}

assetPack {
    packName.set("assetpack_shaders_vulkan")
    dynamicDelivery {
        deliveryType.set("on-demand")
    }
}
