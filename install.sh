# copy files
cp * /path/to/sdk && cd /path/to/sdk

# patching build
cd build && patch -p1 < ../0001-kylin_build_whitelist.patch && cd -

# patching productdefine
cd productdefine/common/ && patch -p1 < ../../0002-productdefine_common_inherit_rich.patch && cd -

# patching kernel build
cd kernel/linux/build/ && patch -p1 < ../../../0003-kernel_linux_build.patch && cd -

# patching hdf
cd drivers/hdf_core/ && patch -p1 < ../../0006-drivers_hdf_core.patch && cd -

# patching graphic_2d
cd foundation/graphic/graphic_2d/ && patch -p1 < ../../../0007-foundation_graphic_graphic_2d_config.patch && cd -

echo "./build.sh --product arm64_virt --ccache --jobs 128 --no-prebuilt-sdk" > build_qemu.sh
chmod +x build_qemu.sh
