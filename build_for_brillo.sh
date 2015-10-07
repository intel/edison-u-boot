set -e

rm -rf out

make O=out CROSS_COMPILE=$ANDROID_BUILD_TOP/prebuilts/gcc/linux-x86/x86/x86_64-linux-android-4.9/bin/x86_64-linux-android- CFLAGS=-m32 -j$(nproc) edison_defconfig
make O=out CROSS_COMPILE=$ANDROID_BUILD_TOP/prebuilts/gcc/linux-x86/x86/x86_64-linux-android-4.9/bin/x86_64-linux-android- CFLAGS=-m32 -j$(nproc)

rm -rf out-fastboot

make O=out-fastboot CROSS_COMPILE=$ANDROID_BUILD_TOP/prebuilts/gcc/linux-x86/x86/x86_64-linux-android-4.9/bin/x86_64-linux-android- CFLAGS=-m32 KCPPFLAGS=-DCONFIG_BRILLO_FASTBOOT_ONLY -j$(nproc) edison_defconfig
make O=out-fastboot CROSS_COMPILE=$ANDROID_BUILD_TOP/prebuilts/gcc/linux-x86/x86/x86_64-linux-android-4.9/bin/x86_64-linux-android- CFLAGS=-m32 KCPPFLAGS=-DCONFIG_BRILLO_FASTBOOT_ONLY -j$(nproc)

dd if=out/u-boot.bin of=u-boot-edison.bin bs=4096 seek=1

openssl base64 -d <<EOF | xz -d > u-boot-edison.img
/Td6WFoAAATm1rRGAgAhARYAAAB0L+Wj4AH/ADJdABITxmI/dj2HEN524/dYfdgu
VSVrkQEE6XSgsZnwdRltLRkU6GTigpK5eAw3CQAaRzcAAAAAtBe6DKrPzUcAAU6A
BAAAAASYg8ixxGf7AgAAAAAEWVo=
EOF
dd if=out-fastboot/u-boot.bin of=u-boot-edison.img bs=4608 seek=1
