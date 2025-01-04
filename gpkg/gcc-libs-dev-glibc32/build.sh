TERMUX_PKG_HOMEPAGE=https://gcc.gnu.org/
TERMUX_PKG_DESCRIPTION="Runtime libraries shipped by GCC on a 32-bit basis (temporary package)"
TERMUX_PKG_LICENSE="GPL-3.0"
TERMUX_PKG_MAINTAINER="@termux-pacman"
TERMUX_PKG_VERSION=14.2.1
TERMUX_PKG_SKIP_SRC_EXTRACT=true
TERMUX_PKG_BLACKLISTED_ARCHES="arm, i686"

termux_step_make_install() {
	(
		termux_set_crosses_arch
		mkdir -p ${TERMUX_LIB32_PATH}
		cp -r ${CGCT_DIR}/${TERMUX_ARCH}/lib/lib* ${TERMUX_LIB32_PATH}
	)
}
