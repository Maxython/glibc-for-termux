TERMUX_PKG_HOMEPAGE=https://gcc.gnu.org/
TERMUX_PKG_DESCRIPTION="Runtime libraries shipped by GCC"
TERMUX_PKG_LICENSE="GPL-3.0"
TERMUX_PKG_MAINTAINER="@termux-pacman"
TERMUX_PKG_VERSION=14.2.2
_COMMIT=65c8fd7b017482c6d6bd0c7a7c6e296f016e38d0
TERMUX_PKG_SRCURL=git+https://sourceware.org/git/gcc
TERMUX_PKG_SHA256=8ac196a7ded1ee3fbd43e8fc374070ff358f790ee93a1d5e7499ab4728968b50
TERMUX_PKG_GIT_BRANCH="master"
#TERMUX_PKG_SRCURL=https://ftp.gnu.org/gnu/gcc/gcc-$TERMUX_PKG_VERSION/gcc-$TERMUX_PKG_VERSION.tar.xz
#TERMUX_PKG_SHA256=a7b39bc69cbf9e25826c5a60ab26477001f7c08d85cec04bc0e29cabed6f3cc9
TERMUX_PKG_DEPENDS="glibc"
TERMUX_PKG_BUILD_DEPENDS="doxygen-glibc"
TERMUX_PKG_BREAKS="gcc-glibc-libs-dev"
TERMUX_PKG_REPLACES="gcc-glibc-libs-dev"
TERMUX_PKG_NO_STATICSPLIT=true
TERMUX_PKG_SEPARATE_SUB_DEPENDS=true
TERMUX_PKG_ONLY_INSTALLING=true

termux_step_post_get_source() {
	git fetch --unshallow
	git checkout $_COMMIT

	local s=$(find . -type f ! -path '*/.git/*' -print0 | xargs -0 sha256sum | LC_ALL=C sort | sha256sum)
	if [[ "${s}" != "${TERMUX_PKG_SHA256}  "* ]]; then
		termux_error_exit "Checksum mismatch for source files."
	fi
}

termux_step_pre_configure() {
	sed -i 's@\./fixinc\.sh@-c true@' ${TERMUX_PKG_SRCDIR}/gcc/Makefile.in
	sed -i '/m64=/s/lib64/lib/' ${TERMUX_PKG_SRCDIR}/gcc/config/i386/t-linux64
	sed -i '/lp64=/s/lib64/lib/' ${TERMUX_PKG_SRCDIR}/gcc/config/aarch64/t-aarch64-linux
	echo "${TERMUX_PKG_VERSION}" > ${TERMUX_PKG_SRCDIR}/gcc/BASE-VER
	echo "${_COMMIT}" > ${TERMUX_PKG_SRCDIR}/gcc/DEV-PHASE
	CFLAGS=${CFLAGS/-Werror=format-security/}
	CXXFLAGS=${CXXFLAGS/-Werror=format-security/}
	CFLAGS+=" -I${TERMUX_PREFIX}/include -L${TERMUX_PREFIX}/lib -g3"
	CXXFLAGS+=" -I${TERMUX_PREFIX}/include -L${TERMUX_PREFIX}/lib -g3"
}

termux_step_configure() {
	local CONFIGFLAG=""
	case $TERMUX_ARCH in
		"aarch64") CONFIGFLAG="--with-arch=armv8-a --enable-fix-cortex-a53-835769 --enable-fix-cortex-a53-843419";;
		"arm") CONFIGFLAG="--with-arch=armv7-a --with-float=hard --with-fpu=neon";;
		"x86_64") CONFIGFLAG="--with-arch=x86-64";;
		"i686") CONFIGFLAG="--with-arch=i686";;
	esac

	${TERMUX_PKG_SRCDIR}/configure \
		--host=$TERMUX_HOST_PLATFORM \
		--build=$TERMUX_HOST_PLATFORM \
		--target=$TERMUX_HOST_PLATFORM \
		--prefix=$TERMUX_PREFIX \
		--libdir=$TERMUX_PREFIX/lib \
		--libexecdir=$TERMUX_PREFIX/lib \
		--mandir=$TERMUX_PREFIX/share/man \
		--infodir=$TERMUX_PREFIX/share/info \
		--with-bugurl=https://github.com/termux-pacman/glibc-packages/issues \
		--with-gmp=$TERMUX_PREFIX \
		--with-mpfr=$TERMUX_PREFIX \
		--with-mpc=$TERMUX_PREFIX \
		--enable-clocale=gnu \
		$CONFIGFLAG \
		--disable-multilib \
		--disable-bootstrap \
		--disable-nls \
		--enable-default-pie \
		--enable-default-ssp \
		--enable-languages=c,c++,fortran \
		--with-system-zlib \
		--enable-__cxa_atexit \
		--enable-linker-build-id \
		--enable-plugin \
		--enable-libstdcxx-backtrace \
		--with-linker-hash-style=gnu \
		--enable-gnu-indirect-function \
		--disable-werror \
		--disable-checking \
		--enable-host-shared \
		--disable-libssp \
		--disable-libstdcxx-pch \
		LD_FOR_TARGET=$TERMUX_PREFIX/bin/ld
}

termux_step_make() {
	make
}

termux_step_make_install() {
	local _libdir=$TERMUX_PREFIX/lib/gcc/$TERMUX_HOST_PLATFORM/${TERMUX_PKG_VERSION%%+*}

	# --- gcc-libs ---
	make -C $TERMUX_HOST_PLATFORM/libgcc install-shared

	for lib in libatomic \
		libgfortran \
		libgomp \
		libitm \
		libquadmath \
		libsanitizer/{a,l,ub}san \
		libstdc++-v3/src \
		libvtv; do
		make -C $TERMUX_HOST_PLATFORM/$lib install-toolexeclibLTLIBRARIES
	done

	if [ "$TERMUX_ARCH" = "x86_64" ] || [ "$TERMUX_ARCH" = "aarch64" ]; then
		make -C $TERMUX_HOST_PLATFORM/libsanitizer/tsan install-toolexeclibLTLIBRARIES
	fi

	make -C $TERMUX_HOST_PLATFORM/libstdc++-v3/po install

	for lib in libgomp \
		libitm \
		libquadmath; do
		make -C $TERMUX_HOST_PLATFORM/$lib install-info
	done

	# --- gcc ---
	make -C gcc install-driver install-cpp install-gcc-ar \
		c++.install-common install-headers install-plugin install-lto-wrapper

	install -m755 -t $TERMUX_PREFIX/bin/ gcc/gcov{,-tool}
	install -m755 -t ${_libdir}/ gcc/{cc1,cc1plus,collect2,lto1}

	make -C $TERMUX_HOST_PLATFORM/libgcc install

	make -C $TERMUX_HOST_PLATFORM/libstdc++-v3/src install
	make -C $TERMUX_HOST_PLATFORM/libstdc++-v3/include install
	make -C $TERMUX_HOST_PLATFORM/libstdc++-v3/libsupc++ install
	make -C $TERMUX_HOST_PLATFORM/libstdc++-v3/python install

	make install-libcc1
	if [ -f $TERMUX_PREFIX/lib/libstdc++.so.6.*-gdb.py ]; then
		install -d $TERMUX_PREFIX/share/gdb/auto-load/usr/lib
		mv $TERMUX_PREFIX/lib/libstdc++.so.6.*-gdb.py \
			$TERMUX_PREFIX/share/gdb/auto-load/usr/lib/
	fi

	make install-fixincludes
	make -C gcc install-mkheaders

	make -C lto-plugin install
	install -dm755 $TERMUX_PREFIX/lib/bfd-plugins/
	ln -s /${_libdir}/liblto_plugin.so \
		$TERMUX_PREFIX/lib/bfd-plugins/

	make -C $TERMUX_HOST_PLATFORM/libgomp install-nodist_{libsubinclude,toolexeclib}HEADERS
	make -C $TERMUX_HOST_PLATFORM/libitm install-nodist_toolexeclibHEADERS
	make -C $TERMUX_HOST_PLATFORM/libquadmath install-nodist_libsubincludeHEADERS
	make -C $TERMUX_HOST_PLATFORM/libsanitizer install-nodist_{saninclude,toolexeclib}HEADERS
	make -C $TERMUX_HOST_PLATFORM/libsanitizer/asan install-nodist_toolexeclibHEADERS
	if [ "$TERMUX_ARCH" = "aarch64" ]; then
		make -C $TERMUX_HOST_PLATFORM/libsanitizer/tsan install-nodist_toolexeclibHEADERS
	fi
	make -C $TERMUX_HOST_PLATFORM/libsanitizer/lsan install-nodist_toolexeclibHEADERS

	make -C gcc install-man install-info

	make -C libcpp install
	make -C gcc install-po

	ln -s gcc $TERMUX_PREFIX/bin/cc

	install -Dm755 $TERMUX_PKG_BUILDER_DIR/c89 $TERMUX_PREFIX/bin/c89
	install -Dm755 $TERMUX_PKG_BUILDER_DIR/c99 $TERMUX_PREFIX/bin/c99

	python -m compileall $TERMUX_PREFIX/share/gcc-${TERMUX_PKG_VERSION%%+*}/
	python -O -m compileall $TERMUX_PREFIX/share/gcc-${TERMUX_PKG_VERSION%%+*}/

	# --- gcc-fortran ---

	make -C $TERMUX_HOST_PLATFORM/libgfortran install-cafexeclibLTLIBRARIES \
		install-{toolexeclibDATA,nodist_fincludeHEADERS,gfor_cHEADERS}
	make -C $TERMUX_HOST_PLATFORM/libgomp install-nodist_fincludeHEADERS
	make -C gcc fortran.install-{common,man,info}
	install -Dm755 gcc/f951 ${_libdir}/f951

	ln -s gfortran $TERMUX_PREFIX/bin/f95

	if [ -d $TERMUX_PREFIX/lib64 ]; then
		mv $TERMUX_PREFIX/lib64/* $TERMUX_PREFIX/lib
		rm -fr $TERMUX_PREFIX/lib64
	fi

	if [ -d $TERMUX_PREFIX/lib32 ]; then
		mv $TERMUX_PREFIX/lib32/* $TERMUX_PREFIX/lib
		rm -fr $TERMUX_PREFIX/lib32
	fi
}
