
function( codesign path deep is_dmg)
IF(APPLE)
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
ENDIF(APPLE)
endfunction()

IF(APPLE)
function( bundlesign path entitlements)
    message(STATUS "Signing ${path} entitlements ${entitlements}")

    set ( args
        --verbose=2
        --force
        --timestamp
        --deep
        --sign "-"
        --options runtime
    )

    if( entitlements )
        list( APPEND args
                --entitlements "${entitlements}"
        )
    endif()
    message(STATUS "bundlesign ${args} ${path}" )
    execute_process( COMMAND xcrun codesign ${args} ${path} )
endfunction()
ELSE
function( bundlesign path)
endfunction()
ENDIF(APPLE)
#message(WARNING "${CPACK_PACKAGE_FILES}  ")
#codesign( "${CPACK_PACKAGE_FILES}" Off On)
