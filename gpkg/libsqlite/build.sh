TERMUX_PKG_HOMEPAGE=https://www.sqlite.org
TERMUX_PKG_DESCRIPTION="A C library that implements an SQL database engine"
TERMUX_PKG_LICENSE="Public Domain"
TERMUX_PKG_MAINTAINER="@termux-pacman"
_SQLITE_SRCVER=3460100
_SQLITE_YEAR=2024
TERMUX_PKG_VERSION=3.47.0
TERMUX_PKG_SRCURL=https://www.sqlite.org/${_SQLITE_YEAR}/sqlite-src-${_SQLITE_SRCVER}.zip
TERMUX_PKG_SHA256=def3fc292eb9ecc444f6c1950e5c79d8462ed5e7b3d605fd6152d145e1d5abb4
TERMUX_PKG_DEPENDS="zlib-glibc"
TERMUX_PKG_EXTRA_CONFIGURE_ARGS="
--disable-static
--enable-fts3
--enable-fts4
--enable-fts5
--enable-rtree
TCLLIBDIR=$TERMUX_PREFIX/lib/sqlite$TERMUX_PKG_VERSION
"

termux_step_pre_configure() {
	export CFLAGS="${CFLAGS/_FORTIFY_SOURCE=3/_FORTIFY_SOURCE=2}"
	export CXXFLAGS="${CXXFLAGS/_FORTIFY_SOURCE=3/_FORTIFY_SOURCE=2}"
	export CPPFLAGS="$CPPFLAGS \
		-DSQLITE_ENABLE_COLUMN_METADATA=1 \
		-DSQLITE_ENABLE_UNLOCK_NOTIFY \
		-DSQLITE_ENABLE_DBSTAT_VTAB=1 \
		-DSQLITE_ENABLE_FTS3_TOKENIZER=1 \
		-DSQLITE_ENABLE_FTS3_PARENTHESIS \
		-DSQLITE_SECURE_DELETE \
		-DSQLITE_ENABLE_STMTVTAB \
		-DSQLITE_MAX_VARIABLE_NUMBER=250000 \
		-DSQLITE_MAX_EXPR_DEPTH=10000 \
		-DSQLITE_ENABLE_MATH_FUNCTIONS"
}

termux_step_make() {
	sed -i -e 's/ -shared / -Wl,-O1,--as-needed\0/g' libtool
	make
	make showdb showjournal showstat4 showwal sqldiff sqlite3_analyzer
}

termux_step_post_make_install() {
	install -m755 showdb showjournal showstat4 showwal sqldiff sqlite3_analyzer lemon $TERMUX_PREFIX/bin/

	install -m755 -d $TERMUX_PREFIX/share/man/man1
	install -m644 $TERMUX_PKG_SRCDIR/sqlite3.1 $TERMUX_PREFIX/share/man/man1/

	install -m755 -d $TERMUX_PREFIX/share/man/mann
	install -m644 $TERMUX_PKG_SRCDIR/autoconf/tea/doc/sqlite3.n $TERMUX_PREFIX/share/man/mann/

	install -Dm644 lempar.c $TERMUX_PREFIX/share/lemon/lempar.c
}
