TERMUX_PKG_HOMEPAGE=https://www.gnu.org/software/wget/
TERMUX_PKG_DESCRIPTION="Commandline tool for retrieving files using HTTP, HTTPS and FTP"
TERMUX_PKG_LICENSE="GPL-3.0"
TERMUX_PKG_MAINTAINER="@termux-pacman"
TERMUX_PKG_VERSION=1.25.0
TERMUX_PKG_SRCURL=https://mirrors.kernel.org/gnu/wget/wget-${TERMUX_PKG_VERSION}.tar.lz
TERMUX_PKG_SHA256=19225cc756b0a088fc81148dc6a40a0c8f329af7fd8483f1c7b2fe50f4e08a1f
TERMUX_PKG_DEPENDS="libidn2-glibc, libuuid-glibc, openssl-glibc, pcre2-glibc, zlib-glibc, libnettle-glibc, libpsl-glibc, libgnutls-glibc"
TERMUX_PKG_CONFFILES="glibc/etc/wgetrc"
TERMUX_PKG_EXTRA_CONFIGURE_ARGS="
--disable-rpath
--enable-nls
--with-ssl=openssl
"
