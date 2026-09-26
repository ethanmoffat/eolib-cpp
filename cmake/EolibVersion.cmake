# Computes EOLIB_VERSION_FULL (SemVer string including any pre-release suffix) from the project version and
# EOLIB_VERSION_SUFFIX.

if(EOLIB_VERSION_SUFFIX)
    if(NOT EOLIB_VERSION_SUFFIX MATCHES "^(alpha|beta|rc)\\.[0-9]+$")
        message(FATAL_ERROR "EOLIB_VERSION_SUFFIX '${EOLIB_VERSION_SUFFIX}' must be of the form alpha.N, beta.N or rc.N")
    endif()
    set(EOLIB_VERSION_FULL "${PROJECT_VERSION}-${EOLIB_VERSION_SUFFIX}")
else()
    set(EOLIB_VERSION_FULL "${PROJECT_VERSION}")
endif()

message(STATUS "eolib version: ${EOLIB_VERSION_FULL}")
