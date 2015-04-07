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

# Executable name and its dependencies
add_executable(v4l2server
	${SOURCE_FILES}
)

# Needed libraries
target_link_libraries(v4l2server
	JderobotInterfaces
	${ZeroCIce_LIBRARIES}
)

