# Maintainer: Sebastian Fedrau <sebastian.fedrau@gmail.com>
pkgname=efind
pkgver=0.2.1
pkgrel=1
epoch=
pkgdesc="An extendable wrapper for GNU find."
arch=('i686' 'x86_64')
url="http://efind.dixieflatline.de"
license=('GPL3')
groups=()
depends=('findutils>=4.2.0' 'ffcall>=1.10' 'python2>=2.7.10')
makedepends=('gcc' 'bison' 'flex' 'make' 'ffcall' 'python2')
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
