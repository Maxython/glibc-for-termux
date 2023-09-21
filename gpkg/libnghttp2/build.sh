TERMUX_PKG_HOMEPAGE=https://nghttp2.org/
TERMUX_PKG_DESCRIPTION="nghttp HTTP 2.0 library"
TERMUX_PKG_LICENSE="MIT"
TERMUX_PKG_MAINTAINER="@termux-pacman"
TERMUX_PKG_VERSION=1.56.0
TERMUX_PKG_SRCURL=https://github.com/nghttp2/nghttp2/releases/download/v${TERMUX_PKG_VERSION}/nghttp2-${TERMUX_PKG_VERSION}.tar.xz
TERMUX_PKG_SHA256=65eee8021e9d3620589a4a4e91ce9983d802b5229f78f3313770e13f4d2720e9
TERMUX_PKG_DEPENDS="glibc"
TERMUX_PKG_EXTRA_CONFIGURE_ARGS="--enable-lib-only"
