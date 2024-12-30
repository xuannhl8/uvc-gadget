SUMMARY = "usb video class gadget"
DESCRIPTION = "UVC gadget application"
AUTHOR = "dinhlee1905@gmail.com"
LICENSE = "CLOSED"

python do_display_banner() {
    bb.plain("***********************************************");
    bb.plain("*                                             *");
    bb.plain("*     Hello World from Elink Engineering!     *");
    bb.plain("*                                             *");
    bb.plain("***********************************************");
}

addtask display_banner before do_build

SRC_URI = "git://github.com/xuannhl8/uvc-gadget.git;protocol=https;branch=main"
SRCREV = "527f5ee609345e5a21f14e94350c7b9563bab048"

DEPENDS = "meson ninja jpeg"
BUILD_DEPENDS = "meson-native ninja-native"

S = "${WORKDIR}/git"

inherit meson pkgconfig

do_install() {
    # usr/bin	 
    install -d ${D}${bindir}
    install -m 755 ${B}/src/uvc-gadget ${D}${bindir}
    # usr/lib
    install -d ${D}${libdir}
    install -m 0644 ${B}/lib/libuvcgadget.so.0.4.0 ${D}${libdir}
    ln -sf libuvcgadget.so.0.4.0 ${D}${libdir}/libuvcgadget.so.0
    ln -sf libuvcgadget.so.0 ${D}${libdir}/libuvcgadget.so
}

FILES_${PN} = "${bindir}/uvc-gadget ${libdir}/libuvcgadget.so ${libdir}/libuvcgadget.so.0 ${libdir}/libuvcgadget.so.0.4.0"
FILES_${PN}-dev = "${libdir}/libuvcgadget.so"
