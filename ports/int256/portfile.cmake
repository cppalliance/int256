# Copyright 2026 Matt Borland
# Distributed under the Boost Software License, Version 1.0.
# https://www.boost.org/LICENSE_1_0.txt
#
# See: https://devblogs.microsoft.com/cppblog/registries-bring-your-own-libraries-to-vcpkg/
#
# The SHA512 below is a placeholder. After tagging v1.0.0 replace it with the
# hash vcpkg reports for the release tarball (vcpkg prints the expected value on
# the first failed install, or run:
#   curl -L https://github.com/cppalliance/int256/archive/refs/tags/v1.0.0.tar.gz | shasum -a 512
# ).

vcpkg_from_github(
        OUT_SOURCE_PATH SOURCE_PATH
        REPO cppalliance/int256
        REF v1.0.0
        SHA512 0
        HEAD_REF develop
)

file(COPY "${SOURCE_PATH}/include" DESTINATION "${CURRENT_PACKAGES_DIR}")
file(INSTALL "${SOURCE_PATH}/LICENSE" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)
