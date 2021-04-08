# Store version into variable
execute_process(COMMAND ${GIT_EXECUTABLE} -C ${GIT_PATH} describe --tags --always --abbrev=4 --dirty
    OUTPUT_VARIABLE GIT_REPO_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE)
# The variable will be used when file is configured
configure_file(${INPUT_FILE} ${OUTPUT_FILE})
