TERMUX_PKG_HOMEPAGE=https://llvm.org/
TERMUX_PKG_DESCRIPTION="Compiler infrastructure"
TERMUX_PKG_LICENSE="Apache-2.0, NCSA"
TERMUX_PKG_LICENSE_FILE="LICENSE.TXT"
TERMUX_PKG_MAINTAINER="@termux-pacman"
TERMUX_PKG_VERSION=18.1.3
_SOURCE=https://github.com/llvm/llvm-project/releases/download/llvmorg-$TERMUX_PKG_VERSION
TERMUX_PKG_SRCURL=($_SOURCE/clang-$TERMUX_PKG_VERSION.src.tar.xz
		$_SOURCE/clang-tools-extra-$TERMUX_PKG_VERSION.src.tar.xz
		$_SOURCE/llvm-$TERMUX_PKG_VERSION.src.tar.xz
		$_SOURCE/cmake-$TERMUX_PKG_VERSION.src.tar.xz
		$_SOURCE/third-party-$TERMUX_PKG_VERSION.src.tar.xz)
TERMUX_PKG_SHA256=(e43e1729713ac0241aa026fa2f98bb54e74a196a6fed60ab4819134a428eb6d8
		e59a804b95e29fcc0b8e496065b0e7b9c9225efaea48294b31c03f1624dedc4e
		fa6db8951f5ef576ac6bad43d5e1ed83962754538c998fbfa0397cd4521abc00
		acfecb615d41c5b1a0a31e15324994ca06f7a3f37d8958d719b20de0d217b71b
		ba1de46e740133d361c0d5d1387befa309f0b60f81bc2bf003252bebdcf9eada)
TERMUX_PKG_DEPENDS="libllvm-glibc, gcc-glibc, compiler-rt-glibc, lld-glibc"
TERMUX_PKG_BUILD_DEPENDS="llvm-glibc, python-glibc"
TERMUX_PKG_NO_STATICSPLIT=true

termux_step_post_get_source() {
	for i in cmake third-party; do
		rm -fr $TERMUX_TOPDIR/$TERMUX_PKG_NAME/${i}
		mv $TERMUX_PKG_SRCDIR/$i-$TERMUX_PKG_VERSION.src $TERMUX_TOPDIR/$TERMUX_PKG_NAME
		mv $TERMUX_TOPDIR/$TERMUX_PKG_NAME/$i-$TERMUX_PKG_VERSION.src $TERMUX_TOPDIR/$TERMUX_PKG_NAME/$i
	done

	mv $TERMUX_PKG_SRCDIR/clang-tools-extra-$TERMUX_PKG_VERSION.src $TERMUX_PKG_SRCDIR/tools/extra
}

termux_step_configure() {
	termux_setup_cmake
	termux_setup_ninja

	local LLVM_TARGET_ARCH="X86"
	if [ $TERMUX_ARCH = "arm" ]; then
		LLVM_TARGET_ARCH="ARM"
	elif [ $TERMUX_ARCH = "aarch64" ]; then
		LLVM_TARGET_ARCH="AArch64"
	fi

	export PATH=$TERMUX_PKG_BUILDDIR/bin:$PATH

	cmake ${TERMUX_PKG_SRCDIR} \
		-G Ninja \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_FIND_ROOT_PATH=$TERMUX_PREFIX \
		-DCMAKE_INSTALL_DOCDIR=share/doc \
		-DCMAKE_INSTALL_PREFIX=$TERMUX_PREFIX \
		-DCMAKE_SYSTEM_NAME=Linux \
		-DLLVM_HOST_TRIPLE=$TERMUX_HOST_PLATFORM \
		-DCMAKE_LIBRARY_ARCHITECTURE=$TERMUX_HOST_PLATFORM \
		-DCMAKE_SYSTEM_PROCESSOR=$LLVM_TARGET_ARCH \
		-DLLVM_TARGETS_TO_BUILD=$LLVM_TARGET_ARCH \
		-DCMAKE_SKIP_RPATH=ON \
		-DCLANG_DEFAULT_PIE_ON_LINUX=ON \
		-DCLANG_LINK_CLANG_DYLIB=ON \
		-DENABLE_LINKER_BUILD_ID=ON \
		-DLLVM_BUILD_DOCS=ON \
		-DLLVM_BUILD_TESTS=ON \
		-DLLVM_ENABLE_RTTI=ON \
		-DLLVM_ENABLE_SPHINX=ON \
		-DLLVM_EXTERNAL_LIT=$TERMUX_PREFIX/bin/lit \
		-DLLVM_INCLUDE_DOCS=ON \
		-DLLVM_LINK_LLVM_DYLIB=ON \
		-DLLVM_MAIN_SRC_DIR=$TERMUX_PKG_SRCDIR/llvm-$TERMUX_PKG_VERSION.src \
		-DSPHINX_WARNINGS_AS_ERRORS=OFF \
		-DLLVM_INCLUDE_TESTS=OFF
}
