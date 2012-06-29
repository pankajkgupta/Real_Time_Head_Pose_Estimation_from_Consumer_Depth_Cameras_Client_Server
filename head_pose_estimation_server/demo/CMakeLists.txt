cmake_minimum_required(VERSION 2.6)

SET( 	HEAD_DEMO
		../CRForestEstimator.cpp
		../CRTree.cpp
		gl_camera.cpp
		main.cpp 
)

SET(CMAKE_BUILD_TYPE "Release")

IF(WIN32)

	SET( OPENCV_INCLUDES "C:\\opencv\\include;C:\\opencv\\modules\\core\\include;C:\\opencv\\modules\\highgui\\include;C:\\opencv\\modules\\imgproc\\include" ) 
	SET( OPENCV_LIB "C:\\opencv\\build\\x64\\vc10\\lib" ) 

	SET( OPENNI_INCLUDES "C:\\Program Files\\OpenNI\\Include" ) 
	SET( OPENNI_LIB "C:\\Program Files\\OpenNI\\Lib64" ) 

	SET( FREEGLUT_LIB "C:\\freeglut-MSVC-2.8.0-1.mp\\freeglut\\lib\\x64" ) 
	LINK_DIRECTORIES( ${OPENNI_LIB} ${OPENCV_LIB} ${FREEGLUT_LIB} )


ELSE(WIN32)

	SET( OPENCV_INCLUDES /include ) 
	SET( OPENCV_LIB /lib ) 
	SET( OPENNI_INCLUDES /home/biwi/gfanelli/OpenNI/Include ) 
	SET( OPENNI_LIB /home/biwi/gfanelli/OpenNI/Platform/Linux86/Bin/Release ) 

	#assuming freeglut installed on linux
	LINK_DIRECTORIES( ${OPENNI_LIB} ${OPENCV_LIB} )

ENDIF(WIN32)
	
INCLUDE_DIRECTORIES( . .. ${OPENNI_INCLUDES} ${OPENCV_INCLUDES} )

	
add_executable( ../head_pose_estimation_demo ${HEAD_DEMO})


IF(WIN32)
target_link_libraries (../head_pose_estimation_demo opencv_core240 opencv_highgui240 opencv_imgproc240 OpenNI64 freeglut)
ELSE(WIN32)
target_link_libraries (../head_pose_estimation_demo opencv_core opencv_highgui opencv_imgproc OpenNI GLU glut)
ENDIF(WIN32)

