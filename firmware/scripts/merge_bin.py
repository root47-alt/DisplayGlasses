# PlatformIO post-build step: writes merged.bin next to firmware.bin.
#
# merged.bin holds the bootloader, partition table and app in one file that
# flashes at offset 0. The browser flasher (web/flash) uses it, so nobody
# needs PlatformIO installed just to put firmware on the glasses.
Import("env")  # noqa: F821  (provided by PlatformIO's SCons)


def merge_bin(source, target, env):
    board = env.BoardConfig()
    parts = []
    for offset, path in env.get("FLASH_EXTRA_IMAGES", []):
        parts += [offset, '"%s"' % path]
    parts += ["$ESP32_APP_OFFSET", '"$BUILD_DIR/${PROGNAME}.bin"']
    cmd = [
        '"$OBJCOPY"', "--chip", board.get("build.mcu", "esp32"), "merge-bin",
        "-o", '"$BUILD_DIR/merged.bin"',
        "--flash-mode", "${__get_board_flash_mode(__env__)}",
        "--flash-size", board.get("upload.flash_size", "4MB"),
    ] + parts
    env.Execute(" ".join(cmd))


env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", merge_bin)  # noqa: F821
