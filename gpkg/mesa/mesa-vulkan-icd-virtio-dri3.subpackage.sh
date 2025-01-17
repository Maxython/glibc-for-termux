TERMUX_SUBPKG_DESCRIPTION="Mesa's Virtio Vulkan ICD"
TERMUX_SUBPKG_DEPEND_ON_PARENT="no"
TERMUX_SUBPKG_DEPENDS="vulkan-icd-loader-glibc, gcc-libs-glibc, zlib-glibc, zstd-glibc, libxcb-glibc, libdrm-glibc, libx11-glibc, libxshmfence-glibc, libexpat-glibc, libwayland-glibc"
TERMUX_SUBPKG_EXCLUDED_ARCHES="arm, i686, x86_64"
TERMUX_SUBPKG_INCLUDE="
glibc/lib/libvulkan_virtio.so
glibc/share/vulkan/icd.d/virtio_icd.*.json
"
