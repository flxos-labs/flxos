# FlxOS Version Embedding
#
# Reads version.txt and injects version information as compile definitions

function(flx_embed_version)
    # Read version from version.txt
    if(EXISTS "${CMAKE_SOURCE_DIR}/version.txt")
        file(READ "${CMAKE_SOURCE_DIR}/version.txt" FLX_VERSION_STRING)
        string(STRIP "${FLX_VERSION_STRING}" FLX_VERSION_STRING)
        
        # Parse semantic version (MAJOR.MINOR.PATCH)
        string(REPLACE "." ";" VERSION_LIST ${FLX_VERSION_STRING})
        list(LENGTH VERSION_LIST VERSION_PARTS)
        
        if(VERSION_PARTS GREATER_EQUAL 1)
            list(GET VERSION_LIST 0 FLX_VERSION_MAJOR)
        else()
            set(FLX_VERSION_MAJOR 0)
        endif()
        
        if(VERSION_PARTS GREATER_EQUAL 2)
            list(GET VERSION_LIST 1 FLX_VERSION_MINOR)
        else()
            set(FLX_VERSION_MINOR 0)
        endif()
        
        if(VERSION_PARTS GREATER_EQUAL 3)
            list(GET VERSION_LIST 2 FLX_VERSION_PATCH)
        else()
            set(FLX_VERSION_PATCH 0)
        endif()
        
        # Add compile definitions
        add_compile_definitions(
            FLX_VERSION_MAJOR=${FLX_VERSION_MAJOR}
            FLX_VERSION_MINOR=${FLX_VERSION_MINOR}
            FLX_VERSION_PATCH=${FLX_VERSION_PATCH}
            FLX_VERSION_STRING="${FLX_VERSION_STRING}"
        )
        
        message(STATUS "FlxOS: Version ${FLX_VERSION_STRING} (${FLX_VERSION_MAJOR}.${FLX_VERSION_MINOR}.${FLX_VERSION_PATCH})")
    else()
        message(WARNING "FlxOS: version.txt not found, using default version 0.0.0")
        add_compile_definitions(
            FLX_VERSION_MAJOR=0
            FLX_VERSION_MINOR=0
            FLX_VERSION_PATCH=0
            FLX_VERSION_STRING="0.0.0"
        )
    endif()
endfunction()
