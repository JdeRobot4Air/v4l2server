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

