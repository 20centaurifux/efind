# Maintainer: Sebastian Fedrau <sebastian.fedrau@gmail.com>
pkgname=efind
pkgver=0.1.0
pkgrel=1
epoch=
pkgdesc="An extendable wrapper for GNU find providing an easier expression syntax."
arch=('i686' 'x86_64')
url="https://github.com/20centaurifux/efind"
license=('GPL3')
groups=()
depends=('findutils>=4.2.0')
makedepends=()
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
