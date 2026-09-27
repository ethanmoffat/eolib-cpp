# eolib_fetch_dependency(<name> <FetchContent_Declare arguments>...)
#
# Declares and populates a FetchContent dependency. With EOLIB_OFFLINE, nothing is downloaded: the sources from an
# earlier online configure of the same build directory are reused, or FETCHCONTENT_SOURCE_DIR_<NAME> can point to a
# local copy.

include(FetchContent)

macro(eolib_fetch_dependency eolib_fetch_name)
    FetchContent_Declare(${eolib_fetch_name} ${ARGN})

    if(EOLIB_OFFLINE)
        set(FETCHCONTENT_FULLY_DISCONNECTED ON)
        string(TOUPPER "${eolib_fetch_name}" eolib_fetch_upper_name)
        string(TOLOWER "${eolib_fetch_name}" eolib_fetch_lower_name)
        if(FETCHCONTENT_SOURCE_DIR_${eolib_fetch_upper_name})
            set(eolib_fetch_source_dir "${FETCHCONTENT_SOURCE_DIR_${eolib_fetch_upper_name}}")
        else()
            set(eolib_fetch_source_dir "${FETCHCONTENT_BASE_DIR}/${eolib_fetch_lower_name}-src")
        endif()
        if(NOT EXISTS "${eolib_fetch_source_dir}")
            message(
                FATAL_ERROR
                    "EOLIB_OFFLINE is ON, but ${eolib_fetch_name} has not been downloaded to ${eolib_fetch_source_dir}. "
                    "Configure this build directory once without EOLIB_OFFLINE, install the system package, or set "
                    "FETCHCONTENT_SOURCE_DIR_${eolib_fetch_upper_name} to a local copy.")
        endif()
    endif()

    FetchContent_MakeAvailable(${eolib_fetch_name})
endmacro()
