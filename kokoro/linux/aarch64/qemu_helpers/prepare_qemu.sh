#!/bin/bash
#
# Setup and configure qemu userspace emulator on kokoro worker so that we can seamlessly emulate processes running
# inside docker containers.

set -ex

# show pre-existing qemu registration
cat /proc/sys/fs/binfmt_misc/qemu-aarch64

# Kokoro ubuntu1604 workers have already qemu-user and qemu-user-static packages installed, but it's and old version that:
# * prints warning about some syscalls (e.g "qemu: Unsupported syscall: 278")
# * doesn't register with binfmt_misc with the "F" flag we need (see below)
# To avoid the "unsupported syscall" warnings, we download a recent version of qemu-aarch64-static.
# On newer distributions (such as glinux), simply "apt install qemu-user-static" is sufficient.
curl -sSL --output qemu-aarch64-static https://github.com/multiarch/qemu-user-static/releases/download/v5.2.0-2/qemu-aarch64-static
chmod ugo+x qemu-aarch64-static
sudo mv qemu-aarch64-static /usr/local/bin
/usr/local/bin/qemu-aarch64-static --version

# register qemu-aarch64-static with binfmt_misc to use it as emulator for aarch64 binaries (based on their magic and mask)
# The format of the configuration string is ":name:type:offset:magic:mask:interpreter:flags"
# The most important flag is "F", which allows the qemu-aarch64-static binary to be loaded regardless of chroot and namespaces.
# That way, we can emulate aarch64 binaries running inside docker containers transparently, without needing the emulator
# to be accessible from the docker image we're emulating.
echo -1 | sudo tee /proc/sys/fs/binfmt_misc/qemu-aarch64  # delete the existing registration
echo ":qemu-aarch64:M::\x7fELF\x02\x01\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x02\x00\xb7\x00:\xff\xff\xff\xff\xff\xff\xff\x00\xff\xff\xff\xff\xff\xff\xff\xff\xfe\xff\xff\xff:/usr/local/bin/qemu-aarch64-static:OCF" | sudo tee /proc/sys/fs/binfmt_misc/register

# Print current qemu reqistration to make sure everything is setup correctly.
cat /proc/sys/fs/binfmt_misc/qemu-aarch64
