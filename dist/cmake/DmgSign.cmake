
function( codesign path deep is_dmg)
    message(STATUS "Signing ${path}")

    set ( args 
        --verbose=2
        --force
        --timestamp
        --sign "-"
    )

    if( NOT is_dmg )
        list( APPEND args
                --options runtime 
                --entitlements "${APPLE_CODESIGN_ENTITLEMENTS}"
        )
    endif()

    if( deep )
        list( APPEND args --deep)
    endif()

    execute_process( COMMAND xcrun codesign ${args} ${path} )
endfunction()
message(WARNING "${CPACK_PACKAGE_FILES}")
codesign( "${CPACK_PACKAGE_FILES}" Off On)
