# Needs to be run from the parent directory of fiasco.git

set -e

. /tools/elpsoft/setgccpath.sh

. gse-mips 4.6.3

export CROSS_COMPILE=mips-mti-linux-gnu-
export ARCH=mips

if [ $# -eq 1 ]; then

git clone https://bitbucket.org/sketch14/fiasco.oc-mips32.git fiasco.git

cd fiasco.git
git checkout elliptic5
cd src/l4/pkg

git clone https://bitbucket.org/sketch14/karma-mips32-vz.git karma

cd karma
git checkout ellipticx
cd ../../..

git clone https://bitbucket.org/sketch14/karma-linux-mti.git linux
cd linux
git checkout elliptic
cd ../../..
fi

cd fiasco.git
cd src/kernel/fiasco
make BUILDDIR=../../../build/fiasco
cp src/templates/globalconfig.out.mips32-malta-vz-nomenu ../../../build/fiasco/globalconfig.out
cd ../../../build/fiasco/
make -j2

cd ../../src/kernel/fiasco
make BUILDDIR=../../../build-guest/fiasco
cp src/templates/globalconfig.out.mips32-karma-nomenu ../../../build-guest/fiasco/globalconfig.out
cd ../../../build-guest/fiasco/
make -j2

cd ../../src/l4
make B=../../build-guest/l4 'DROPSCONF_DEFCONFIG=$(L4DIR)/mk/defconfig/config.guestos'
mkdir ../../build-guest/l4/images

if [ ! -d ../../build-guest/l4/conf ]; then
  mkdir ../../build-guest/l4/conf
fi

#if [ ! -d ../../build/l4/conf ]; then
#  mkdir -p ../../build/l4/conf
#fi

cp conf/platforms/mips/Makeconf_guest.boot ../../build-guest/l4/conf/Makeconf.boot
#cp conf/Makeconf_host.boot ../../build/l4/conf/Makeconf.boot
cd ../../build-guest/l4

make -j2

make E=hello S=bootstrap

cd ../../src/l4
make B=../../build/l4 'DROPSCONF_DEFCONFIG=$(L4DIR)/mk/defconfig/config.hostos'
mkdir ../../build/l4/images

#cd ../../build/l4
#make -j2

if [ ! -d ../../build/l4/conf ]; then
  mkdir -p ../../build/l4/conf
fi

cp conf/platforms/mips/Makeconf_host.boot ../../build/l4/conf/Makeconf.boot

cd ../../build/l4
#make S=karma
make

#VAR_PATH=`readlink -f ../../src/l4/pkg/karma`
#sed -i "s@\"\/path\/to\/l4\/pkg\/karma\"@\"$VAR_PATH\"@" .config

mkdir -p ../../build/linux
cd ../../src/linux
make O=../../build/linux karma_defconfig
cd ../../build/linux

VAR_PATH=`readlink -f ../../src/l4/pkg/karma`
sed -i "s@\"\/path\/to\/l4\/pkg\/karma\"@\"$VAR_PATH\"@" .config

make

cd ../../build/l4
make E=karma_fiasco_sys S=bootstrap


