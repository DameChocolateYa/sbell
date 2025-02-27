pkgname=sbell
pkgver=1.0.0
pkgrel=1
pkgdesc="A minimalist interpreter"
arch=('x86_64')
license=('GPL')
depends=('git')
source=("git+https://github.com/DameChocolateYa/sbell.git#branch=main")
md5sums=('SKIP')

#By DameChocolateYa

build() {
    cd "${pkgname}"
    make
}

package() {
    cd "${pkgname}"
    install -Dm755 sbell "$pkgdir/usr/bin/sbell"

    install -d "$pkgdir/etc/sbell/lang"
    install -Dm644 "lang/*" "$pkgdir/etc/sbell/lang/"
}
