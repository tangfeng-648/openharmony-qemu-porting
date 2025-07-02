#!/bin/bash

#Copyright (c) 2020-2021 Huawei Device Co., Ltd.
#Licensed under the Apache License, Version 2.0 (the "License");
#you may not use this file except in compliance with the License.
#You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
#Unless required by applicable law or agreed to in writing, software
#distributed under the License is distributed on an "AS IS" BASIS,
#WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#See the License for the specific language governing permissions and
#limitations under the License.

qemu_option="-M virt -cpu cortex-a53 -smp 4 -m 8192 -nographic"
qemu_setup_network=""
qemu_instance_id=""
img_copy_option="-n"

#kernel_bootargs="loglevel=7 ip=192.168.100.2:192.168.100.1:192.168.100.1:255.255.255.0::eth0:off sn=0023456789 console=ttyAMA0,115200 init=/bin/init ohos.boot.hardware=qemu.arm64.linux default_boot_device=a003e00.virtio_mmio root=/dev/ram0 rw ohos.required_mount.system=/dev/block/vdb@/usr@ext4@ro,barrier=1@wait,required ohos.required_mount.vendor=/dev/block/vdc@/vendor@ext4@ro,barrier=1@wait,required ohos.required_mount.misc=/dev/block/vda@/misc@none@none=@wait,required ohos.required_mount.data=/dev/block/vdd@/data/@ext4@nosuid,nodev,noatime,barrier=1,data=ordered,noauto_da_alloc@wait,reservedsize=104857600 ohos.required_mount.sys_prod=/dev/block/vde@/sys_prod@ext4@ro,barrier=1@wait,required ohos.required_mount.chip_prod=/dev/block/vdf@/chip_prod@ext4@ro,barrier=1@wait,required"
kernel_bootargs="loglevel=7 ip=192.168.10.2:192.168.10.1:192.168.10.1:255.255.255.0::eth0:off sn=0023456789 console=ttyAMA0,115200 init=/bin/init ohos.boot.hardware=arm64_virt default_boot_device=a003e00.virtio_mmio root=/dev/ram0 rw ohos.required_mount.system=/dev/block/vdb@/usr@ext4@ro,barrier=1@wait,required ohos.required_mount.vendor=/dev/block/vdc@/vendor@ext4@ro,barrier=1@wait,required ohos.required_mount.misc=/dev/block/vda@/misc@none@none=@wait,required ohos.required_mount.data=/dev/block/vdd@/data/@ext4@nosuid,nodev,noatime,barrier=1,data=ordered,noauto_da_alloc@wait,reservedsize=104857600"

elf_file="out/arm64_virt/packages/phone/images"
if [ -f "${PWD}/ramdisk.img" ]; then
  elf_file="${PWD}"
fi

help_info=$(cat <<-END
Usage: qemu-run [OPTION]...
Run a OHOS image in qemu according to the options.
    -e,  --exec image_path    images path, including: zImage-dtb, ramdisk.img, system.img, vendor.img, userdata.img
    -g,  --gdb                enable gdb for kernel.
    -n,  --network            auto setup network for qemu (sudo required).
    -i,  --instance id        start qemu images with specified instance id (from 01 to 99).
                              it will also setup network when running in multiple instance mode.
         -f                   force override instance image with a new copy.
    -h,  --help               print this help info.

    If no image_path specified, it will find OHOS image in current working directory; then try ${elf_file}.

    When setting up network, it will create br0 on the host PC with the following information:
        IP address: 192.168.100.1
        netmask: 255.255.255.0

    The default qemu device MAC address is [00:22:33:44:55:66], default serial number is [0023456789].
    When running in multiple instances mode, the MAC address and serial number will increase with specified instance ID as follow:
        MAC address:    {instanceID}:22:33:44:55:66
        Serial number:  {instanceID}23456789
END
)

function echo_help()
{
    echo "${help_info}"
    exit 0
}

function qemu_option_add()
{
    qemu_option+=" "
    qemu_option+="$1"
}

function kernel_bootargs_add()
{
    kernel_bootargs+=" "
    kernel_bootargs+="$1"
}

function setup_sn()
{
    if [ x"${qemu_instance_id}" != x ]; then
        kernel_bootargs_add "sn=${qemu_instance_id}23456789"
    else
        kernel_bootargs_add "sn=0023456789"
    fi
}

function parameter_verification()
{
    if [ $1 -eq 0 ] || [ x"$(echo $2 | cut -c 1)" = x"-" ]; then
        echo_help
    fi
}

function qemu_network()
{
    ifconfig br0 > /dev/null 2>&1
    echo 'allow all' | sudo tee -a /etc/qemu/bridge.conf
    sudo modprobe tun tap
    sudo ip link add br0 type bridge
    sudo ip address add 192.168.137.1/24 dev br0
    sudo ip link set dev br0 up

    qemu_option_add "-netdev bridge,id=net0 -device virtio-net-device,netdev=net0,mac=00:22:33:44:55:66"
    qemu_option_add "-monitor telnet:127.0.0.1:55555,server,nowait"
}

function copy_img()
{
    cp ${img_copy_option} $elf_file/userdata.img $elf_file/userdata${qemu_instance_id}.img
    cp ${img_copy_option} $elf_file/vendor.img $elf_file/vendor${qemu_instance_id}.img
    cp ${img_copy_option} $elf_file/system.img $elf_file/system${qemu_instance_id}.img
    cp ${img_copy_option} $elf_file/updater.img $elf_file/updater${qemu_instance_id}.img
}

while [ $# -ne 0 ]
do
    case $1 in
        "-e")
            shift
            parameter_verification $# $1
            elf_file="$1"
            ;;
        "-i")
            shift
            parameter_verification $# $1
            qemu_instance_id=$1
            # Setup qemu network by default when running in multi instance mode
            qemu_setup_network="y"
            ;;
        "-n")
            qemu_setup_network="y"
            ;;
        "-g")
            qemu_option_add "-s -S"
            ;;
        "-f")
            img_copy_option=""
            ;;
        "-h")
            echo_help
            ;;
    esac

    shift
done

if [ ! -f $elf_file/ramdisk.img ]; then
  echo_help
fi

# Setup qemu network if
[ x"${qemu_setup_network}" != x ] && qemu_network

# Copy original images for multi instance running
[ x"${qemu_instance_id}" != x ] && copy_img

setup_sn

# qemu_option_add "-drive if=none,file=$elf_file/chip_prod${qemu_instance_id}.img,format=raw,id=chip_prod,index=5 -device virtio-blk-device,drive=chip_prod"
# qemu_option_add "-drive if=none,file=$elf_file/sys_prod${qemu_instance_id}.img,format=raw,id=sys_prod,index=4 -device virtio-blk-device,drive=sys_prod"
qemu_option_add "-drive if=none,file=$elf_file/userdata${qemu_instance_id}.img,format=raw,id=userdata,index=3 -device virtio-blk-device,drive=userdata"
qemu_option_add "-drive if=none,file=$elf_file/vendor${qemu_instance_id}.img,format=raw,id=vendor,index=2 -device virtio-blk-device,drive=vendor"
qemu_option_add "-drive if=none,file=$elf_file/system${qemu_instance_id}.img,format=raw,id=system,index=1 -device virtio-blk-device,drive=system"
qemu_option_add "-drive if=none,file=$elf_file/updater${qemu_instance_id}.img,format=raw,id=updater,index=0 -device virtio-blk-device,drive=updater"
qemu_option_add "-kernel $elf_file/Image -initrd $elf_file/ramdisk.img"

qemu_option_add "-vnc :20 -device virtio-gpu-pci,xres=486,yres=864,addr=08.0"
# qemu_option_add "-vga none -vnc :20"
# qemu_option_add "-device virtio-mouse-pci -device virtio-keyboard-pci"
qemu_option_add "-device usb-ehci,id=usb,addr=0x4 -device usb-tablet -device usb-mouse"
#qemu_option_add "-k en-us -display sdl,gl=off"
qemu_option_add "-k en-us"
qemu_option_add "-net nic -net tap,ifname=tap0,script=no"

# Setup network need sudo
[ x"${qemu_setup_network}" != x ] && sudo qemu-system-arm64 ${qemu_option} -append "${kernel_bootargs}"

# start without sudo if no need to setup network
[ x"${qemu_setup_network}" == x ] && qemu-system-arm64 ${qemu_option} -append "${kernel_bootargs}"
