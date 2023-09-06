TERMUX_PKG_HOMEPAGE=http://savannah.nongnu.org/projects/attr/
TERMUX_PKG_DESCRIPTION="Utilities for manipulating filesystem extended attributes"
TERMUX_PKG_LICENSE="GPL-2.0"
TERMUX_PKG_MAINTAINER="@termux-pacman"
TERMUX_PKG_VERSION=2.5.1
TERMUX_PKG_REVISION=1
TERMUX_PKG_SRCURL=http://download.savannah.gnu.org/releases/attr/attr-${TERMUX_PKG_VERSION}.tar.gz
TERMUX_PKG_SHA256=bae1c6949b258a0d68001367ce0c741cebdacdd3b62965d17e5eb23cd78adaf8
TERMUX_PKG_DEPENDS="glibc, gcc-glibc-libs-dev"
TERMUX_PKG_BUILD_IN_SRC=true
# At the moment we don't have gettext-glibc
TERMUX_PKG_EXTRA_CONFIGURE_ARGS="--enable-gettext=no"
