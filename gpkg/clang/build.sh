TERMUX_PKG_HOMEPAGE=https://llvm.org/
TERMUX_PKG_DESCRIPTION="Compiler infrastructure"
TERMUX_PKG_LICENSE="Apache-2.0, NCSA"
TERMUX_PKG_LICENSE_FILE="LICENSE.TXT"
TERMUX_PKG_MAINTAINER="@termux-pacman"
TERMUX_PKG_VERSION=18.1.4
_SOURCE=https://github.com/llvm/llvm-project/releases/download/llvmorg-$TERMUX_PKG_VERSION
TERMUX_PKG_SRCURL=($_SOURCE/clang-$TERMUX_PKG_VERSION.src.tar.xz
		$_SOURCE/clang-tools-extra-$TERMUX_PKG_VERSION.src.tar.xz
		$_SOURCE/llvm-$TERMUX_PKG_VERSION.src.tar.xz
		$_SOURCE/cmake-$TERMUX_PKG_VERSION.src.tar.xz
		$_SOURCE/third-party-$TERMUX_PKG_VERSION.src.tar.xz)
TERMUX_PKG_SHA256=(5b11ddda23d3e6d5c17f0d20acdfa8890100d35120e99b4786fdcf4f36593c5c
		2cc806943af96f3391afc404765c936944a1fa441b752723c67fc3614f4f2ee7
		954df1e7a7768ec0c9804da75e5332d68bcc7396c475faf6ed77e7150e4bcdcd
		1acdd829b77f658ba4473757178f9960abcb6ac8d2c700b0772a952b3c9306ba
		270c2f49625c98d53fa1c17a1d4da412c93e729c3a0468304a6915b19dcd8448)
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
