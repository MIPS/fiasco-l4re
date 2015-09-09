#!/bin/bash
# Setup and build Fiasco/L4Re with Karma VMM, Linux and Fiasco guest VMs.
#
# Refer to HOWTO.fiasco-l4re for more information or to help trouble-shooting.
#
# This script uses HTTPS to clone from GitHub. Consider caching your GitHub
# password in Git in order to avoid having to enter it manually during the
# script execution. Or alternatively, change to using SSH. See:
#
# https://help.github.com/articles/caching-your-github-password-in-git/#platform-linux
#
# Preconditions:
# set CROSS_COMPILE=<toolchain-prefix-> and set ARCH=mips
# add toolchain to $PATH
: ${CROSS_COMPILE:?"Set CROSS_COMPILE=<toolchain-prefix->"}
: ${ARCH:?"Set ARCH= (i.e. mips)"}

set -e

if [[ $# -ne 1 ]]; then
    echo
    echo "Run from the parent directory of the Fiasco/L4Re git repository."
    echo "Usage:" $0 "<fiasco_l4re_dir>"
    echo "  or:  DO_SMP=1" $0 "<fiasco_l4re_dir>"
    echo
    echo "Script to setup, git clone, checkout and build Fiasco/L4Re with Karma VMM,"
    echo "Linux and Fiasco guest VMs. When <fiasco_l4re_dir> already exists the clone"
    echo "and checkout are skipped. Optionally, call with DO_SMP=1 to build smp."
    exit 1
fi

# Set default DO_SMP if env var not set
: ${DO_SMP:=0}

# Set default branch and config files
if [[ $DO_SMP -ne 1 ]]; then
    karma_linux_branch=karma_mips_3.10.14
    fiasco_host_globalconfig=globalconfig.out.mips32-malta-vz
    l4re_bootstrap_entry=karma_fiasco_sys
    karma_linux_defconfig=karma_defconfig
else
    karma_linux_branch=karma_mips_4.0.4
    fiasco_host_globalconfig=globalconfig.out.mips32-malta-smp-vz
    l4re_bootstrap_entry=karma_smp_sys
    karma_linux_defconfig=karma_paravirt_defconfig
fi
fiasco_l4re_branch=master
karma_branch=master

# Get absolute path for Fisaco/l4Re git repository
fiasco_l4re_dir="$1"
fiasco_l4re_abs=""
if [[ ! -d $fiasco_l4re_dir ]]; then
    mkdir $fiasco_l4re_dir
fi

fiasco_l4re_abs=`readlink -f $fiasco_l4re_dir`
if [[ ! -d $fiasco_l4re_abs ]]; then
    echo "Failed to get absolute path of fiasco_l4re_dir"
    exit 1
fi

function clone_and_checkout() {
    if [[ ! -d $fiasco_l4re_abs/src ]]; then
        cd $fiasco_l4re_abs/..
        git clone https://github.com/MIPS/fiasco-l4re.git $fiasco_l4re_dir
        cd $fiasco_l4re_dir
        git checkout $fiasco_l4re_branch
    fi

    if [[ ! -d $fiasco_l4re_abs/src/l4/pkg/karma ]]; then
        cd $fiasco_l4re_abs/src/l4/pkg
        git clone https://github.com/MIPS/karma.git karma
        cd karma
        git checkout $karma_branch
    fi

    if [[ ! -d $fiasco_l4re_abs/src/linux ]]; then
        cd $fiasco_l4re_abs/src
        git clone https://github.com/MIPS/karma-linux-mti.git linux
        cd linux
        git checkout $karma_linux_branch
    fi
}

function build_fiasco() {
    build="$1"
    defconfig="$2"
    if [[ ! -d $fiasco_l4re_abs/$build/fiasco ]]; then
        cd $fiasco_l4re_abs/src/kernel/fiasco
        make BUILDDIR=../../../$build/fiasco
        cp src/templates/$defconfig ../../../$build/fiasco/globalconfig.out
        cd ../../../$build/fiasco/
        make olddefconfig
    fi
    cd $fiasco_l4re_abs/$build/fiasco
    make -j4
}

function build_l4re() {
    build="$1"
    defconfig="$2"
    makeconf_boot="$3"
    if [[ ! -d $fiasco_l4re_abs/$build/l4 ]]; then
        cd $fiasco_l4re_abs/src/l4
        make B=../../$build/l4 $defconfig
        mkdir -p ../../$build/l4/images
        mkdir -p ../../$build/l4/conf
        cp $fiasco_l4re_abs/src/l4/$makeconf_boot ../../$build/l4/conf/Makeconf.boot
    fi
    cd $fiasco_l4re_abs/$build/l4
    make -j4
}

function build_linux() {
    build="$1"
    defconfig="$2"
    build_smp=$3
    if [[ ! -d $fiasco_l4re_abs/$build/linux ]]; then
        cd $fiasco_l4re_abs/src/linux
        mkdir $fiasco_l4re_abs/$build/linux
        make O=../../$build/linux $defconfig
        cd $fiasco_l4re_abs/$build/linux

        # Set path to karma directory for include files
        karma_path=$fiasco_l4re_abs/src/l4/pkg/karma
        sed -i "s@\"\/path\/to\/l4\/pkg\/karma\"@\"$karma_path\"@" .config
    fi

    cd $fiasco_l4re_abs/$build/linux
    make -j4
    if [[ $build_smp -eq 1 ]]; then
        if [[ ! -f vmlinux-paravirt ]]; then
            ln -s vmlinux vmlinux-paravirt
        fi
    fi

}

echo "### Starting git clone and checkout ###"
clone_and_checkout

echo "### Building host fiasco kernel ###"
build_fiasco build $fiasco_host_globalconfig

echo "### Building guest fiasco kernel ###"
build_fiasco build-guest globalconfig.out.mips32-karma

echo "### Building guest L4Re ###"
build_l4re build-guest 'DROPSCONF_DEFCONFIG=$(L4DIR)/mk/defconfig/config.mips.guestos' conf/platforms/mips/Makeconf_guest.boot

echo "### Building guest fiasco VM image ###"
cd $fiasco_l4re_abs/build-guest/l4
make E=hello S=bootstrap

echo "### Building host L4Re ###"
build_l4re build 'DROPSCONF_DEFCONFIG=$(L4DIR)/mk/defconfig/config.mips.hostos' conf/platforms/mips/Makeconf_host.boot

echo "### Building guest linux VM image ###"
build_linux build $karma_linux_defconfig $DO_SMP

echo "### Building host bootstrap image ###"
cd $fiasco_l4re_abs/build/l4
make E=$l4re_bootstrap_entry S=bootstrap

echo "### Build finished ###"
