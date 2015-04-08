# Source code files
set(SOURCE_FILES
	v4l2server.cpp
)

# Include headers from this paths
include_directories(
	${INTERFACES_CPP_DIR}
	${LIBS_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(v4l2 SHARED
	v4l2.cpp
)

set_property(TARGET v4l2 PROPERTY SOVERSION 0.1.0)


# Executable name and its dependencies
add_executable(v4l2server
	${SOURCE_FILES}
)

# Needed libraries
target_link_libraries(v4l2server
	v4l2
	JderobotInterfaces
	${ZeroCIce_LIBRARIES}
)

# Generate documentation if doxygen was found
if(DOXYGEN_FOUND)
    get_filename_component(DOC_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
    # Uncomment this line if this library/module has its own customized Doxygen.in file
    #set(DOXYGEN_INPUT ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
    set(DOXYGEN_INPUT ${DOXYGEN_DEFAULT_INPUT})
    set(DOXYGEN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile)
    set(DOXYGEN_OUTPUT ${DOXYGEN_OUTPUT_BASE}/components/${DOC_NAME}/)
    file(MAKE_DIRECTORY ${DOXYGEN_OUTPUT})
    configure_file(${DOXYGEN_INPUT} ${DOXYGEN_FILE} @ONLY)
    add_custom_target(doc_${DOC_NAME} ${DOXYGEN_EXECUTABLE} ${DOXYGEN_FILE} | grep "Parsing file"
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating: component ${DOC_NAME}" VERBATIM
        DEPENDS ${DOXYGEN_INPUT}
    )
    add_dependencies(doc doc_${DOC_NAME})
endif(DOXYGEN_FOUND)

