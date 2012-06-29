cmake_minimum_required(VERSION 2.6)

SET( 	HEAD_DEMO
		CRForestEstimator.cpp
		CRTree.cpp
		main.cpp 
)

SET(CMAKE_BUILD_TYPE "Release")

#modify according to your opencv installation
SET( OPENCV_INCLUDES "C:\\opencv\\include;C:\opencv\modules\core\include;C:\opencv\modules\highgui\include;C:\opencv\modules\imgproc\include" ) 
SET( OPENCV_LIB "C:\opencv\build\x86\mingw\lib" ) 
#SET( OPENCV_LIB /lib ) 

INCLUDE_DIRECTORIES( . ${OPENCV_INCLUDES} )
LINK_DIRECTORIES( ${OPENCV_LIB} )

add_executable( head_pose_estimation ${HEAD_DEMO})

target_link_libraries (head_pose_estimation libopencv_core240.dll libopencv_highgui240.dll libopencv_imgproc240.dll )




