IF(GIT_FOUND)
  EXECUTE_PROCESS(
       COMMAND ${GIT_EXECUTABLE} describe --tags --dirty --always --abbrev=4
       WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
       OUTPUT_VARIABLE GIT_REPO_VERSION
       RESULT_VARIABLE GIT_DESCRIBE_RESULT
       ERROR_VARIABLE GIT_DESCRIBE_ERROR
       OUTPUT_STRIP_TRAILING_WHITESPACE
   )
    EXECUTE_PROCESS(
         COMMAND ${GIT_EXECUTABLE} log -n 1 --pretty=format:"%H"
         WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
         OUTPUT_VARIABLE GIT_HASH
         RESULT_VARIABLE GIT_HASH_RESULT
         ERROR_VARIABLE GIT_HASH_ERROR
         OUTPUT_STRIP_TRAILING_WHITESPACE
     )

ELSE(GIT_FOUND)
  SET(GIT_DESCRIBE_RESULT -1)
ENDIF(GIT_FOUND)


STRING(REGEX REPLACE "v([0-9]*)\\.([0-9]*)\\.(.*)"
       "\\1" VERSION_MAJOR "${GIT_REPO_VERSION}")
STRING(REGEX REPLACE "v([0-9]*)\\.([0-9]*)\\.(.*)"
       "\\2" VERSION_MINOR "${GIT_REPO_VERSION}")
STRING(REGEX REPLACE "v([0-9]*)\\.([0-9]*)\\.(.*)"
       "\\3" VERSION_PATCH "${GIT_REPO_VERSION}")

# Debug
#message(STATUS "GIT_REPO_VERSION=${GIT_REPO_VERSION} RES=${GIT_DESCRIBE_RESULT} ERR=${GIT_DESCRIBE_ERROR} MAJOR=${VERSION_MAJOR} MINOR=${VERSION_MINOR} PATCH=${VERSION_PATCH}")
