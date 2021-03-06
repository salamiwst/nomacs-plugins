# If you want to use prefix paths with cmake, copy and rename this file to CMakeUser.txt
# Do not add this file to GIT!

# set your preferred Qt Library path
IF (CMAKE_CL_64)
	SET(CMAKE_PREFIX_PATH "C:/Qt/qt-everywhere-opensource-src-4.8.5-x64-native-gestures/bin/")
ELSE ()
	SET(CMAKE_PREFIX_PATH "C:/Qt/qt-everywhere-opensource-src-4.8.5-x86-native-gestures/bin/")
ENDIF ()

# set your preferred OpenCV Library path
IF (CMAKE_CL_64)
	SET(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "C:/VSProjects/OpenCV/build2012x64")
ELSE ()
	SET(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "C:/VSProjects/OpenCV/build2012x86")
ENDIF ()

# set your preferred nomacs Library path
IF (CMAKE_CL_64)
	SET(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "C:/VSProjects/nomacs.git/build2012x64")
ELSE ()
	SET(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "C:/VSProjects/nomacs.git/build2012x86")
ENDIF ()

