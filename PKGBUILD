# Maintainer: Sebastian Fedrau <sebastian.fedrau@gmail.com>
pkgname=efind
pkgver=0.5.6
pkgrel=1
epoch=
pkgdesc="An extendable wrapper for GNU find."
arch=('i686' 'x86_64')
url="http://efind.dixieflatline.de"
license=('GPL3')
groups=()
depends=('findutils>=4.2.0' 'libffi>=3.2.1' 'python>=3.5.0')
makedepends=('gcc' 'bison' 'flex' 'make' 'libffi' 'python3' 'gettext')
checkdepends=()
optdepends=()
provides=()
conflicts=()
replaces=()
backup=()
options=()
install=
changelog=
source=("$pkgname-$pkgver.tar.xz")
noextract=()
md5sums=('')
validpgpkeys=()

build() {
	cd "$pkgname-$pkgver"
	make
}

package() {
	cd "$pkgname-$pkgver"
	make DESTDIR="$pkgdir/" install
}
