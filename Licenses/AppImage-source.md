# AppImage launcher sources

The launcher is a separate executable; it mounts/extracts the application and
executes AppRun. It is not linked with the game or Unreal Engine.

- Runtime: https://github.com/AppImage/type2-runtime
- Build instructions and pinned dependencies: the repository's build scripts.
- musl: https://git.musl-libc.org/cgit/musl/tree/COPYRIGHT
- libfuse (LGPL 2): https://github.com/libfuse/libfuse/blob/master/LGPL2.txt
- squashfuse: https://github.com/vasi/squashfuse/blob/master/LICENSE
- zstd: https://github.com/facebook/zstd/blob/dev/LICENSE
- zlib: https://zlib.net/zlib_license.html

To replace the launcher, extract the AppDir using `--appimage-extract`, build a
modified type-2 runtime from source, and pass it to appimagetool with
`--runtime-file your-runtime squashfs-root rebuilt.AppImage`. The game does not
check the launcher signature and works from an extracted writable directory.
The release's build inventory records the exact distributed runtime SHA-256.
