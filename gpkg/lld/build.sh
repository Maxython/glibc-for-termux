TERMUX_PKG_HOMEPAGE=https://llvm.org/
TERMUX_PKG_DESCRIPTION="Linker from the LLVM project"
TERMUX_PKG_LICENSE="Apache-2.0, NCSA"
TERMUX_PKG_LICENSE_FILE="LICENSE.TXT"
TERMUX_PKG_MAINTAINER="@termux-pacman"
TERMUX_PKG_VERSION=18.1.4
TERMUX_PKG_REVISION=1
_SOURCE=https://github.com/llvm/llvm-project/releases/download/llvmorg-$TERMUX_PKG_VERSION
TERMUX_PKG_SRCURL=($_SOURCE/lld-$TERMUX_PKG_VERSION.src.tar.xz
		$_SOURCE/llvm-$TERMUX_PKG_VERSION.src.tar.xz
		$_SOURCE/libunwind-$TERMUX_PKG_VERSION.src.tar.xz
                $_SOURCE/cmake-$TERMUX_PKG_VERSION.src.tar.xz)
TERMUX_PKG_SHA256=(bdfd737b899cccfae7cf5fe8a109e7c87844168855d14374aadf33d99493f9f9
                954df1e7a7768ec0c9804da75e5332d68bcc7396c475faf6ed77e7150e4bcdcd
                9e754cec4d3aeebc8ce697c08948ad4fcd37dfe34e51099acf944d09385d7ff3
		1acdd829b77f658ba4473757178f9960abcb6ac8d2c700b0772a952b3c9306ba)
TERMUX_PKG_DEPENDS="libllvm-glibc, gcc-libs-glibc, zlib-glibc, zstd-glibc"
TERMUX_PKG_BUILD_DEPENDS="llvm-glibc"
TERMUX_PKG_EXTRA_CONFIGURE_ARGS="
-DLLVM_HOST_TRIPLE=$TERMUX_HOST_PLATFORM
-DCMAKE_LIBRARY_ARCHITECTURE=$TERMUX_HOST_PLATFORM
-DCMAKE_INSTALL_DOCDIR=share/doc
-DCMAKE_SKIP_RPATH=ON
-DBUILD_SHARED_LIBS=ON
-DLLVM_BUILD_DOCS=ON
-DLLVM_ENABLE_SPHINX=ON
-DLLVM_EXTERNAL_LIT=$TERMUX_PREFIX/bin/lit
-DLLVM_MAIN_SRC_DIR=$TERMUX_TOPDIR/$TERMUX_PKG_NAME/llvm
-DLLVM_INCLUDE_TESTS=OFF
-DLLVM_LINK_LLVM_DYLIB=ON
-DSPHINX_WARNINGS_AS_ERRORS=OFF
"

termux_step_post_get_source() {
	for i in llvm libunwind cmake; do
		rm -fr $TERMUX_TOPDIR/$TERMUX_PKG_NAME/${i}
		mv $TERMUX_PKG_SRCDIR/$i-$TERMUX_PKG_VERSION.src $TERMUX_TOPDIR/$TERMUX_PKG_NAME
		mv $TERMUX_TOPDIR/$TERMUX_PKG_NAME/$i-$TERMUX_PKG_VERSION.src $TERMUX_TOPDIR/$TERMUX_PKG_NAME/$i
	done
}

termux_step_pre_configure() {
	local LLVM_TARGET_ARCH="X86"
	if [ $TERMUX_ARCH = "arm" ]; then
		LLVM_TARGET_ARCH="ARM"
	elif [ $TERMUX_ARCH = "aarch64" ]; then
		LLVM_TARGET_ARCH="AArch64"
	fi

	TERMUX_PKG_EXTRA_CONFIGURE_ARGS+=" -DCMAKE_SYSTEM_PROCESSOR=${LLVM_TARGET_ARCH}"
}
