#!/bin/bash

set -e

if [[ -z "${1// }" ]]
then
    echo "To sign u-boot, run build_for_brillo.sh --sign."
    echo "Building u-boot..."
    sleep 2
else
if [ "$1" == "--sign" ]
then
    echo "Building u-boot for signing..."
    sleep 2
else
    echo "To sign u-boot, run build_for_brillo.sh --sign"
    exit
fi
fi

rm -rf out

make O=out CROSS_COMPILE=$ANDROID_BUILD_TOP/prebuilts/gcc/linux-x86/x86/x86_64-linux-android-4.9/bin/x86_64-linux-android- CFLAGS=-m32 -j$(nproc) edison_defconfig
make O=out CROSS_COMPILE=$ANDROID_BUILD_TOP/prebuilts/gcc/linux-x86/x86/x86_64-linux-android-4.9/bin/x86_64-linux-android- CFLAGS=-m32 -j$(nproc)

rm -rf out-fastboot

if [ "$1" == "--sign" ]
then
    make O=out-fastboot CROSS_COMPILE=$ANDROID_BUILD_TOP/prebuilts/gcc/linux-x86/x86/x86_64-linux-android-4.9/bin/x86_64-linux-android- CFLAGS=-m32 KCPPFLAGS=-DCONFIG_BRILLO_FASTBOOT_ONLY KCPPFLAGS=-DCONFIG_OSIP_SIGNED_ATTRIBUTE -j$(nproc) edison_defconfig
    make O=out-fastboot CROSS_COMPILE=$ANDROID_BUILD_TOP/prebuilts/gcc/linux-x86/x86/x86_64-linux-android-4.9/bin/x86_64-linux-android- CFLAGS=-m32 KCPPFLAGS=-DCONFIG_BRILLO_FASTBOOT_ONLY KCPPFLAGS=-DCONFIG_OSIP_SIGNED_ATTRIBUTE -j$(nproc)
else
    make O=out-fastboot CROSS_COMPILE=$ANDROID_BUILD_TOP/prebuilts/gcc/linux-x86/x86/x86_64-linux-android-4.9/bin/x86_64-linux-android- CFLAGS=-m32 KCPPFLAGS=-DCONFIG_BRILLO_FASTBOOT_ONLY -j$(nproc) edison_defconfig
    make O=out-fastboot CROSS_COMPILE=$ANDROID_BUILD_TOP/prebuilts/gcc/linux-x86/x86/x86_64-linux-android-4.9/bin/x86_64-linux-android- CFLAGS=-m32 KCPPFLAGS=-DCONFIG_BRILLO_FASTBOOT_ONLY -j$(nproc)
fi

dd if=out/u-boot.bin of=u-boot-edison.bin bs=4096 seek=1
if [ "$1" == "--sign" ]
then
    dd if=out-fastboot/u-boot.bin of=u-boot-edison.img obs=4096 seek=1
    echo "Use Image Studio Tool to sign u-boot-edison.bin and u-boot-edison.img."
else
    openssl base64 -d <<EOF | xz -d > u-boot-edison.img
    /Td6WFoAAATm1rRGAgAhARYAAAB0L+Wj4AH/ADJdABITxmI/dj2HEN524/dYfdgu
    VSVrkQEE6XSgsZnwdRltLRkU6GTigpK5eAw3CQAaRzcAAAAAtBe6DKrPzUcAAU6A
    BAAAAASYg8ixxGf7AgAAAAAEWVo=
EOF
    dd if=out-fastboot/u-boot.bin of=u-boot-edison.img bs=4608 seek=1
fi
