#!/bin/sh

# Skrypt wykorzystywany do budowania Toolchaina

BINUTILS_VERSION="2.20.1"
GCC_VERSION="4.5.0"
NEWLIB_VERSION="1.18.0"
NCURSES_VERSION="5.7"

# Jezeli pracujemy pod Idylla i architektóry się zgadzają, nie ma co budować toolchaina
if [ "$(uname -s)" = "IdyllaOS" ] && [ "${IDYLLAOS_ARCH}" = "$(uname -m)" ];
then
	echo "You don't need toolchain when you running IdyllaOS"
	exit 0
fi

TMPDIR="${IDYLLAOS_TOOLCHAIN}/tmp"
PREFIX="${IDYLLAOS_TOOLCHAIN}"
BUILD=$(gcc -dumpmachine)

# Funkcja do pobierania plikow
getfile()
{
	if [ ! -e "${TMPDIR}/${2}" ]
	then
		wget -c -O "${TMPDIR}/${2}.part" ${1} && \
		mv "${TMPDIR}/${2}.part" "${TMPDIR}/${2}"
	fi
}

build_binutils()
{
	getfile "http://ftp.gnu.org/gnu/binutils/binutils-${BINUTILS_VERSION}.tar.bz2" "binutils-${BINUTILS_VERSION}.tar.bz2" && \
	SRCDIR="${TMPDIR}/binutils-${BINUTILS_VERSION}" && \
	BUILDDIR="${TMPDIR}/build-binutils" && \
	mkdir -p ${BUILDDIR} && \
	echo "Extracting sources from binutils-${BINUTILS_VERSION}.tar.bz2..." && \
	rm -rf ${SRCDIR} && \
	tar xjf "${TMPDIR}/binutils-${BINUTILS_VERSION}.tar.bz2" -C "${TMPDIR}" && \
	echo "Patching sources:" && \
	patch -d ${SRCDIR} -p1 < "${IDYLLAOS_PKGSRC}/dev-utils/binutils/binutils-${BINUTILS_VERSION}.patch" && \
	echo "Building sources" && \
	cd ${BUILDDIR} && \
	${SRCDIR}/configure --prefix=${PREFIX} --disable-nls --target=${IDYLLAOS_ARCH}-pc-idyllaos && \
	make all && \
	make install
}

build_gcc()
{
	getfile "http://ftp.gnu.org/gnu/gcc/gcc-${GCC_VERSION}/gcc-core-${GCC_VERSION}.tar.bz2" "gcc-core-${GCC_VERSION}.tar.bz2" && \
	getfile "http://ftp.gnu.org/gnu/gcc/gcc-${GCC_VERSION}/gcc-g++-${GCC_VERSION}.tar.bz2" "gcc-g++-${GCC_VERSION}.tar.bz2" && \
	SRCDIR="${TMPDIR}/gcc-${GCC_VERSION}" && \
	BUILDDIR="${TMPDIR}/build-gcc" && \
	mkdir -p ${BUILDDIR} && \
	rm -rf ${SRCDIR} && \
	echo "Extracting sources from gcc-core-${GCC_VERSION}.tar.bz2..." && \
	tar xjf "${TMPDIR}/gcc-core-${GCC_VERSION}.tar.bz2" -C "${TMPDIR}" && \
	echo "Extracting sources from gcc-g++-${GCC_VERSION}.tar.bz2..." && \
	tar xjf "${TMPDIR}/gcc-g++-${GCC_VERSION}.tar.bz2" -C "${TMPDIR}" && \
	echo "Patching sources:" && \
	patch -d ${SRCDIR} -p1 < "${IDYLLAOS_PKGSRC}/dev-utils/gcc/gcc-${GCC_VERSION}.patch" && \
	echo "Building sources" && \
	cd ${BUILDDIR} && \
	${SRCDIR}/configure --prefix=${PREFIX} --disable-nls --target=${IDYLLAOS_ARCH}-pc-idyllaos --disable-multilib --enable-languages=c,c++ && \
	make all-gcc all-target-libgcc && \
	make install-gcc install-target-libgcc
}

build_newlib()
{
	SRCDIR="${IDYLLAOS_PKGSRC}/sys-libs/newlib/newlib-${NEWLIB_VERSION}" && \
	BUILDDIR="${TMPDIR}/build-newlib" && \
	mkdir -p ${BUILDDIR} && \
	echo "Building sources" && \
	cd ${BUILDDIR} && \
	${SRCDIR}/configure --prefix=${PREFIX} --target=${IDYLLAOS_ARCH}-pc-idyllaos &&  \
	make all && \
	make install
}

build_ncurses()
{
	getfile "http://ftp.gnu.org/gnu/ncurses/ncurses-${NCURSES_VERSION}.tar.gz" "ncurses-${NCURSES_VERSION}.tar.gz" && \
	SRCDIR="${TMPDIR}/ncurses-${NCURSES_VERSION}" && \
	BUILDDIR="${TMPDIR}/build-ncurses" && \
	mkdir -p ${BUILDDIR} && \
	echo "Extracting sources from ncurses-${NCURSES_VERSION}.tar.gz..." && \
	rm -rf ${SRCDIR} && \
	tar xzf "${TMPDIR}/ncurses-${NCURSES_VERSION}.tar.gz" -C "${TMPDIR}" && \
	echo "Patching sources:" && \
	patch -d ${SRCDIR} -p1 < "${IDYLLAOS_PKGSRC}/sys-libs/ncurses/ncurses-${NCURSES_VERSION}.patch" && \
	echo "Building sources" && \
	cd ${BUILDDIR}
	${SRCDIR}/configure --prefix=${PREFIX} --disable-nls --prefix=${PREFIX}/${IDYLLAOS_ARCH}-pc-idyllaos \
	--without-cxx --without-cxx-binding --datadir=/usr/share --sysconfdir=/etc --host=${IDYLLAOS_ARCH}-pc-idyllaos \
	--build=${BUILD}
	make all && \
	make install.libs install.includes
}

BUILD_GCC=0
BUILD_BINUTILS=0
BUILD_NEWLIB=0
BUILD_NCURSES=0

while [ -n "$1" ]
do
	case "$1" in
		"all") BUILD_GCC=1; BUILD_BINUTILS=1; BUILD_NEWLIB=1; BUILD_NCURSES=1 ;;
		"binutils") BUILD_BINUTILS=1 ;;
		"gcc") BUILD_GCC=1 ;;
		"newlib") BUILD_NEWLIB=1 ;;
		"ncurses") BUILD_NCURSES=1 ;;
	esac
	shift
done

if [ ${BUILD_GCC} -eq 0 ] && [ ${BUILD_BINUTILS} -eq 0 ] && [ ${BUILD_NEWLIB} -eq 0 ] && [ ${BUILD_NCURSES} -eq 0 ]
then
	echo "Usage: mktoolchain.sh [all | [gcc | binutils | newlib | ncurses]]"
	exit 1
fi

mkdir -p "${TMPDIR}"

[ ${BUILD_BINUTILS} -gt 0 ] && build_binutils
[ ${BUILD_GCC} -gt 0 ] && build_gcc
[ ${BUILD_NEWLIB} -gt 0 ] && build_newlib
[ ${BUILD_NCURSES} -gt 0 ] && build_ncurses

