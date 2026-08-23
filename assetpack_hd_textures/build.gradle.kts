plugins {
    id("com.android.asset-pack")
}

assetPack {
    packName.set("assetpack_hd_textures")
    dynamicDelivery {
        deliveryType.set("on-demand")
    }
}
