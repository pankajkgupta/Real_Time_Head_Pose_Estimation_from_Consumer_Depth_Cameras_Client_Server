/****************************************************************************
*                                                                           *
*  OpenNI 1.x Alpha                                                         *
*  Copyright (C) 2011 PrimeSense Ltd.                                       *
*                                                                           *
*  This file is part of OpenNI.                                             *
*                                                                           *
*  OpenNI is free software: you can redistribute it and/or modify           *
*  it under the terms of the GNU Lesser General Public License as published *
*  by the Free Software Foundation, either version 3 of the License, or     *
*  (at your option) any later version.                                      *
*                                                                           *
*  OpenNI is distributed in the hope that it will be useful,                *
*  but WITHOUT ANY WARRANTY; without even the implied warranty of           *
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the             *
*  GNU Lesser General Public License for more details.                      *
*                                                                           *
*  You should have received a copy of the GNU Lesser General Public License *
*  along with OpenNI. If not, see <http://www.gnu.org/licenses/>.           *
*                                                                           *
****************************************************************************/
//---------------------------------------------------------------------------
// Includes
//---------------------------------------------------------------------------
#include <math.h>
#include "SceneDrawer.h"
#include "Common.h"
#include "Util.h"
#include "ServerCom.h"
#include "MovieMng.h"
#include "SkelMng.h"

#ifndef USE_GLES
#if (XN_PLATFORM == XN_PLATFORM_MACOSX)
	#include <GLUT/glut.h>
#else
	#include <GL/glut.h>
#endif
#else
	#include "opengles.h"
#endif

extern xn::Context g_Context;
extern xn::Context g_ContextR;
extern xn::UserGenerator g_UserGenerator;
extern xn::DepthGenerator g_DepthGenerator;
extern xn::ImageGenerator g_ImageGenerator;

extern ServerCom g_srvCom;

extern XnBool g_bDrawBackground;
extern XnBool g_bDrawPixels;
extern XnBool g_bDrawSkeleton;
extern XnBool g_bPrintID;
extern XnBool g_bPrintState;

extern XnBool g_bPrintFrameID;
extern XnBool g_bMarkJoints;

extern Util g_util;

extern MovieMng g_MovieMng;
extern SkelMng g_SkelMng;

XnRGB24Pixel* g_pTexMap = NULL;

// 送信骨格情報
/*
XnSkeletonJoint g_SendSkelJoints[] = {XN_SKEL_WAIST, XN_SKEL_TORSO, XN_SKEL_NECK, XN_SKEL_HEAD, 
						XN_SKEL_LEFT_SHOULDER, XN_SKEL_LEFT_ELBOW, XN_SKEL_LEFT_WRIST, XN_SKEL_LEFT_HAND, 
						XN_SKEL_RIGHT_SHOULDER, XN_SKEL_RIGHT_ELBOW, XN_SKEL_RIGHT_WRIST, XN_SKEL_RIGHT_HAND, 
						XN_SKEL_LEFT_HIP, XN_SKEL_LEFT_KNEE, XN_SKEL_LEFT_ANKLE, XN_SKEL_LEFT_FOOT, 
						XN_SKEL_RIGHT_HIP, XN_SKEL_RIGHT_KNEE, XN_SKEL_RIGHT_ANKLE, XN_SKEL_RIGHT_FOOT,
						(XnSkeletonJoint)-1};
*/
/*
XnSkeletonJoint g_SendSkelJoints[] = {XN_SKEL_TORSO, XN_SKEL_TORSO, XN_SKEL_NECK, XN_SKEL_HEAD, 
						XN_SKEL_LEFT_SHOULDER, XN_SKEL_LEFT_ELBOW, XN_SKEL_LEFT_HAND, XN_SKEL_LEFT_HAND, 
						XN_SKEL_RIGHT_SHOULDER, XN_SKEL_RIGHT_ELBOW, XN_SKEL_RIGHT_HAND, XN_SKEL_RIGHT_HAND, 
						XN_SKEL_LEFT_HIP, XN_SKEL_LEFT_KNEE, XN_SKEL_LEFT_FOOT, XN_SKEL_LEFT_FOOT, 
						XN_SKEL_RIGHT_HIP, XN_SKEL_RIGHT_KNEE, XN_SKEL_RIGHT_FOOT, XN_SKEL_RIGHT_FOOT,
						(XnSkeletonJoint)-1};
*/
/**/
// 20*3
XnSkeletonJoint g_SendSkelJoints[] = {XN_SKEL_TORSO, XN_SKEL_NECK, XN_SKEL_HEAD, 
						XN_SKEL_LEFT_SHOULDER, XN_SKEL_LEFT_ELBOW, XN_SKEL_LEFT_HAND, 
						XN_SKEL_RIGHT_SHOULDER, XN_SKEL_RIGHT_ELBOW, XN_SKEL_RIGHT_HAND, 
						XN_SKEL_LEFT_HIP, XN_SKEL_LEFT_KNEE, XN_SKEL_LEFT_FOOT, 
						XN_SKEL_RIGHT_HIP, XN_SKEL_RIGHT_KNEE, XN_SKEL_RIGHT_FOOT,
						(XnSkeletonJoint)-1 };
/**/

/*
// 全骨格情報
XnSkeletonJoint g_SendSkelJoints[] = {XN_SKEL_HEAD, XN_SKEL_NECK, XN_SKEL_TORSO, XN_SKEL_WAIST,  
						XN_SKEL_LEFT_COLLAR,  XN_SKEL_LEFT_SHOULDER,  XN_SKEL_LEFT_ELBOW,  XN_SKEL_LEFT_WRIST,  XN_SKEL_LEFT_HAND,  XN_SKEL_LEFT_FINGERTIP, 
						XN_SKEL_RIGHT_COLLAR, XN_SKEL_RIGHT_SHOULDER, XN_SKEL_RIGHT_ELBOW, XN_SKEL_RIGHT_WRIST, XN_SKEL_RIGHT_HAND, XN_SKEL_RIGHT_FINGERTIP, 
						XN_SKEL_LEFT_HIP,  XN_SKEL_LEFT_KNEE,  XN_SKEL_LEFT_ANKLE,  XN_SKEL_LEFT_FOOT, 
						XN_SKEL_RIGHT_HIP, XN_SKEL_RIGHT_KNEE, XN_SKEL_RIGHT_ANKLE, XN_SKEL_RIGHT_FOOT,
						(XnSkeletonJoint)-1};
*/

#include <map>
std::map<XnUInt32, std::pair<XnCalibrationStatus, XnPoseDetectionStatus> > m_Errors;
void XN_CALLBACK_TYPE MyCalibrationInProgress(xn::SkeletonCapability& /*capability*/, XnUserID id, XnCalibrationStatus calibrationError, void* /*pCookie*/)
{
	m_Errors[id].first = calibrationError;
}
void XN_CALLBACK_TYPE MyPoseInProgress(xn::PoseDetectionCapability& /*capability*/, const XnChar* /*strPose*/, XnUserID id, XnPoseDetectionStatus poseError, void* /*pCookie*/)
{
	m_Errors[id].second = poseError;
}

unsigned int getClosestPowerOfTwo(unsigned int n)
{
	unsigned int m = 2;
	while(m < n) m<<=1;

	return m;
}
GLuint initTexture(void** buf, int& width, int& height)
{
	GLuint texID = 0;
	glGenTextures(1,&texID);

	width = getClosestPowerOfTwo(width);
	height = getClosestPowerOfTwo(height); 
	*buf = new unsigned char[width*height*4];
	glBindTexture(GL_TEXTURE_2D,texID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	return texID;
}

GLfloat texcoords[8];
void DrawRectangle(float topLeftX, float topLeftY, float bottomRightX, float bottomRightY)
{
	GLfloat verts[8] = {	topLeftX, topLeftY,
		topLeftX, bottomRightY,
		bottomRightX, bottomRightY,
		bottomRightX, topLeftY
	};
	glVertexPointer(2, GL_FLOAT, 0, verts);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	//TODO: Maybe glFinish needed here instead - if there's some bad graphics crap
	glFlush();
}
void DrawTexture(float topLeftX, float topLeftY, float bottomRightX, float bottomRightY)
{
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, 0, texcoords);

	DrawRectangle(topLeftX, topLeftY, bottomRightX, bottomRightY);

	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

XnFloat Colors[][3] =
{
	{0,1,1},
	{0,0,1},
	{0,1,0},
	{1,1,0},
	{1,0,0},
	{1,.5,0},
	{.5,1,0},
	{0,.5,1},
	{.5,0,1},
	{1,1,.5},
	{1,1,1}
};
XnUInt32 nColors = 10;
#ifndef USE_GLES
void glPrintString(void *font, char *str)
{
	int i,l = (int)strlen(str);

	for(i=0; i<l; i++)
	{
		glutBitmapCharacter(font,*str++);
	}
}
#endif
bool DrawLimb(XnUserID player, XnSkeletonJoint eJoint1, XnSkeletonJoint eJoint2)
{
	if (!g_UserGenerator.GetSkeletonCap().IsTracking(player))
	{
		printf("not tracked!\n");
		return true;
	}

	if (!g_UserGenerator.GetSkeletonCap().IsJointActive(eJoint1) ||
		!g_UserGenerator.GetSkeletonCap().IsJointActive(eJoint2))
	{
		return false;
	}

	XnSkeletonJointPosition joint1, joint2;
	g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(player, eJoint1, joint1);
	g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(player, eJoint2, joint2);

	if (joint1.fConfidence < 0.5 || joint2.fConfidence < 0.5)
	{
		return true;
	}

	XnPoint3D pt[2];
	pt[0] = joint1.position;
	pt[1] = joint2.position;

	g_DepthGenerator.ConvertRealWorldToProjective(2, pt, pt);
#ifndef USE_GLES
//	glVertex3i(pt[0].X, pt[0].Y, 0);
//	glVertex3i(pt[1].X, pt[1].Y, 0);
	glVertex3i(pt[0].X/2, pt[0].Y, 0);
	glVertex3i(pt[1].X/2, pt[1].Y, 0);
#else
	GLfloat verts[4] = {pt[0].X, pt[0].Y, pt[1].X, pt[1].Y};
	glVertexPointer(2, GL_FLOAT, 0, verts);
	glDrawArrays(GL_LINES, 0, 2);
	glFlush();
#endif

	return true;
}

static const float DEG2RAD = 3.14159/180;
 
void drawCircle(float x, float y, float radius)
{
   glBegin(GL_TRIANGLE_FAN);
 
   for (int i=0; i < 360; i++)
   {
      float degInRad = i*DEG2RAD;
      glVertex2f(x + cos(degInRad)*radius, y + sin(degInRad)*radius);
   }
 
   glEnd();
}

bool GetJoint(const XnUserID *player, const XnSkeletonJoint *eJoint, XnPoint3D *pt)
{
	/* 上位でチェック済みなので必要なし
	if (!g_UserGenerator.GetSkeletonCap().IsTracking(*player))
	{
		printf("[%d] not Tracking.\n", __LINE__);
		return false;
	}
	*/

	if (!g_UserGenerator.GetSkeletonCap().IsJointActive(*eJoint))
	{
		//printf("[%d] Joint:%d not Active.\n", __LINE__, *eJoint);
		return false;
	}

	// 座標取得
	XnSkeletonJointPosition joint;
	g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(*player, *eJoint, joint);

	if (joint.fConfidence < 0.5)
	{
		//printf("[%d] Joint:%d fConfidence:%f.\n", __LINE__, *eJoint, joint.fConfidence);
		return false;
	}

	*pt = joint.position;

	return true;
}

void DrawJoint(XnUserID player, XnSkeletonJoint eJoint)
{
//	printf("[%d] eJoint:%d\n", __LINE__, (int)eJoint);
/*
	if (!g_UserGenerator.GetSkeletonCap().IsTracking(player))
	{
		printf("not tracked!\n");
		return;
	}

	if (!g_UserGenerator.GetSkeletonCap().IsJointActive(eJoint))
	{
//		printf("[%d] not Active.\n", __LINE__);
		return;
	}

	XnSkeletonJointPosition joint;
	g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(player, eJoint, joint);

	if (joint.fConfidence < 0.5)
	{
		return;
	}
*/

	XnPoint3D pt;
	GetJoint(&player, &eJoint, &pt);
//	pt = joint.position;

	g_DepthGenerator.ConvertRealWorldToProjective(1, &pt, &pt);

	//drawCircle(pt.X, pt.Y, 2);
	drawCircle(pt.X/2, pt.Y, 2);
}

bool SetJointData(XnUserID player, XnSkeletonJoint eJoint, float *dst)
{
	//printf("[%d] eJoint:%d\n", __LINE__, (int)eJoint);
	XnPoint3D pt;
	if(GetJoint(&player, &eJoint, &pt) == false)
	{
		// 無効値等の場合は0設定
		pt.X = pt.Y = pt.Z = 0;

		// 1つでも無効値があればデータ送信しない
		//return false;
	}

	// 単位mmなので-1から1の間に変換
	*dst = pt.X/1000;
	*(dst+1) = pt.Y/1000;
	*(dst+2) = pt.Z/1000;
	//printf("player:%d joint:%d x:%f y:%f z:%f\n", player, (int)eJoint, pt.X, pt.Y, pt.Z);

	return true;
}

const XnChar* GetCalibrationErrorString(XnCalibrationStatus error)
{
	switch (error)
	{
	case XN_CALIBRATION_STATUS_OK:
		return "OK";
	case XN_CALIBRATION_STATUS_NO_USER:
		return "NoUser";
	case XN_CALIBRATION_STATUS_ARM:
		return "Arm";
	case XN_CALIBRATION_STATUS_LEG:
		return "Leg";
	case XN_CALIBRATION_STATUS_HEAD:
		return "Head";
	case XN_CALIBRATION_STATUS_TORSO:
		return "Torso";
	case XN_CALIBRATION_STATUS_TOP_FOV:
		return "Top FOV";
	case XN_CALIBRATION_STATUS_SIDE_FOV:
		return "Side FOV";
	case XN_CALIBRATION_STATUS_POSE:
		return "Pose";
	default:
		return "Unknown";
	}
}
const XnChar* GetPoseErrorString(XnPoseDetectionStatus error)
{
	switch (error)
	{
	case XN_POSE_DETECTION_STATUS_OK:
		return "OK";
	case XN_POSE_DETECTION_STATUS_NO_USER:
		return "NoUser";
	case XN_POSE_DETECTION_STATUS_TOP_FOV:
		return "Top FOV";
	case XN_POSE_DETECTION_STATUS_SIDE_FOV:
		return "Side FOV";
	case XN_POSE_DETECTION_STATUS_ERROR:
		return "General error";
	default:
		return "Unknown";
	}
}

void DrawMap(const xn::DepthMetaData& dmd, const xn::SceneMetaData& smd, const xn::ImageMetaData& imd)
{
	static bool bInitialized = false;	
	static GLuint depthTexID;
	static GLuint imageTexID;
	static unsigned char* pDepthTexBuf;
	static unsigned char* pImageTexBuf;
	static int texWidth, texHeight;

	float topLeftX;
	float topLeftY;
	float bottomRightY;
	float bottomRightX;
	float texXpos;
	float texYpos;

	static XnUInt64 nLastDepthTime = 0;
	static XnUInt64 nLastImageTime = 0;
	static XnUInt64 nLastUserTime = 0;
	static XnUInt32 nMissedDepthFrames = 0;
	static XnUInt32 nMissedImageFrames = 0;
	static XnUInt32 nMissedUserFrames = 0;
	static XnUInt32 nDepthFrames = 0;
	static XnUInt32 nImageFrames = 0;
	static XnUInt32 nUserFrames = 0;

	if(!bInitialized)
	{
		texWidth =  getClosestPowerOfTwo(dmd.XRes());
		texHeight = getClosestPowerOfTwo(dmd.YRes());
		//printf("texWidth:%d texHeight:%d\n", texWidth, texHeight);

//		printf("Initializing depth texture: width = %d, height = %d\n", texWidth, texHeight);
		depthTexID = initTexture((void**)&pDepthTexBuf,texWidth, texHeight) ;
		imageTexID = initTexture((void**)&pImageTexBuf,texWidth, texHeight) ;

//		printf("Initialized depth texture: width = %d, height = %d\n", texWidth, texHeight);
		bInitialized = true;

		topLeftX = dmd.XRes();
		topLeftY = 0;
		bottomRightY = dmd.YRes();
		bottomRightX = 0;
		texXpos =(float)dmd.XRes()/texWidth;
		texYpos  =(float)dmd.YRes()/texHeight;

		memset(texcoords, 0, 8*sizeof(float));
		texcoords[0] = texXpos, texcoords[1] = texYpos, texcoords[2] = texXpos, texcoords[7] = texYpos;

		g_pTexMap = (XnRGB24Pixel*)malloc(texWidth * texHeight * sizeof(XnRGB24Pixel));
	}

	if(imd.IsDataNew() == 0 || dmd.IsDataNew() == 0 || smd.IsDataNew() == 0)
	{
		printf("Image:%d Depth:%d User:%d\n", imd.IsDataNew(), dmd.IsDataNew(), smd.IsDataNew());
	}

	unsigned int nValue = 0;
	unsigned int nHistValue = 0;
	unsigned int nIndex = 0;
	unsigned int nX = 0;
	unsigned int nY = 0;
	unsigned int nNumberOfPoints = 0;
	XnUInt16 g_nXRes = dmd.XRes();
	XnUInt16 g_nYRes = dmd.YRes();

	unsigned char* pDestImage = pDepthTexBuf;

	const XnDepthPixel* pDepth = dmd.Data();
	const XnLabel* pLabels = smd.Data();

	static unsigned int nZRes = dmd.ZRes();
	static float* pDepthHist = (float*)malloc(nZRes* sizeof(float));

	// Calculate the accumulative histogram
	memset(pDepthHist, 0, nZRes*sizeof(float));
	for (nY=0; nY<g_nYRes; nY++)
	{
		for (nX=0; nX<g_nXRes; nX++)
		{
			nValue = *pDepth;

			if (nValue != 0)
			{
				pDepthHist[nValue]++;
				nNumberOfPoints++;
			}

			pDepth++;
		}
	}

	for (nIndex=1; nIndex<nZRes; nIndex++)
	{
		pDepthHist[nIndex] += pDepthHist[nIndex-1];
	}
	if (nNumberOfPoints)
	{
		for (nIndex=1; nIndex<nZRes; nIndex++)
		{
			pDepthHist[nIndex] = (unsigned int)(256 * (1.0f - (pDepthHist[nIndex] / nNumberOfPoints)));
		}
	}

	pDepth = dmd.Data();
	if (g_bDrawPixels)
	{
		XnUInt32 nIndex = 0;
		// Prepare the texture map
		for (nY=0; nY<g_nYRes; nY++)
		{
			for (nX=0; nX < g_nXRes; nX++, nIndex++)
			{

				pDestImage[0] = 0;
				pDestImage[1] = 0;
				pDestImage[2] = 0;
				if (g_bDrawBackground || *pLabels != 0)
				{
					nValue = *pDepth;
					XnLabel label = *pLabels;
					XnUInt32 nColorID = label % nColors;
					if (label == 0)
					{
						nColorID = nColors;
					}

					if (nValue != 0)
					{
						nHistValue = pDepthHist[nValue];

						pDestImage[0] = nHistValue * Colors[nColorID][0]; 
						pDestImage[1] = nHistValue * Colors[nColorID][1];
						pDestImage[2] = nHistValue * Colors[nColorID][2];
					}
				}

				pDepth++;
				pLabels++;
				pDestImage+=3;
			}

			pDestImage += (texWidth - g_nXRes) *3;
		}
	}
	else
	{
		xnOSMemSet(pDepthTexBuf, 0, 3*2*g_nXRes*g_nYRes);
	}

	xnOSMemSet(g_pTexMap, 0, texWidth*texHeight*sizeof(XnRGB24Pixel));
	if (g_bDrawPixels)
	{
		if (imd.PixelFormat() != XN_PIXEL_FORMAT_RGB24)
		{
			// RGB以外は非対応...
			return;
		}
		const XnRGB24Pixel* pImageRow = imd.RGB24Data();
		XnRGB24Pixel* pTexRow = g_pTexMap + imd.YOffset() * texWidth;

		for (XnUInt y = 0; y < imd.YRes(); ++y)
		{
			const XnRGB24Pixel* pImage = pImageRow;
			XnRGB24Pixel* pTex = pTexRow + imd.XOffset();

			for (XnUInt x = 0; x < imd.XRes(); ++x, ++pImage, ++pTex)
			{
				*pTex = *pImage;
			}

			pImageRow += imd.XRes();
			pTexRow += texWidth;
		}
	}
	else
	{
		xnOSMemSet(g_pTexMap, 0, texWidth*texHeight*sizeof(XnRGB24Pixel));
	}

	// Depth
	glBindTexture(GL_TEXTURE_2D, depthTexID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texWidth, texHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, pDepthTexBuf);

	// Display the OpenGL texture map
	glColor4f(0.75,0.75,0.75,1);

	glEnable(GL_TEXTURE_2D);
	//DrawTexture(dmd.XRes(),dmd.YRes(),0,0);
	DrawTexture(dmd.XRes()/2,dmd.YRes(),0,0);
	glDisable(GL_TEXTURE_2D);


	// Image
	glBindTexture(GL_TEXTURE_2D, imageTexID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texWidth, texHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, g_pTexMap);
	// Display the OpenGL texture map
	//glColor4f(0.75,0.75,0.75,1);
	glColor4f(1,1,1,1);

	glEnable(GL_TEXTURE_2D);
	//DrawTexture(dmd.XRes(),dmd.YRes(),0,0);
	DrawTexture(dmd.XRes(),dmd.YRes(),dmd.XRes()/2,0);
	glDisable(GL_TEXTURE_2D);


	char strLabel[50] = "";
	XnUserID aUsers[15];
	XnUInt16 nUsers = 15;
	g_UserGenerator.GetUsers(aUsers, nUsers);
	

	// フレーム落ちチェック
	XnUInt64 nTimestamp;

	nTimestamp = g_UserGenerator.GetTimestamp();
	Util::CheckMissedFrame("user", nTimestamp, &nUserFrames, &nLastUserTime, &nMissedUserFrames);
	char time_stamp[24];
	g_util.GetTimeStr(nTimestamp, sizeof(time_stamp), time_stamp);

#ifdef REALTIME_FPS_CHECK_ENABLE
	nTimestamp = g_DepthGenerator.GetTimestamp();
	Util::CheckMissedFrame("depth(realtime)", nTimestamp, &nDepthFrames, &nLastDepthTime, &nMissedDepthFrames);
	
	nTimestamp = g_ImageGenerator.GetTimestamp();
	Util::CheckMissedFrame("image(realtime)", nTimestamp, &nImageFrames, &nLastImageTime, &nMissedImageFrames);
#endif

	// データ保存
	g_MovieMng.Update(dmd, imd);

	// 取得スケルトン数カウント
	int  skelNum = 0;
	for (int i = 0; i < nUsers; ++i)
	{
		if(g_UserGenerator.GetSkeletonCap().IsTracking(aUsers[i]))
		{
			skelNum++;
		}
	}

	float values[SRV_SEND_INFO_NUM];
	memset(values, 0, sizeof(values));
	int skelCnt = 0;
	if( skelNum != 0)
	{
		//printf("取得スケルトン数:%d\n", skelNum);
		values[0] = g_SkelMng.GetKinectId();
		values[4] = skelNum;

		// FPS表示
		xnOSMemSet(strLabel, 0, sizeof(strLabel));
		XnUInt32 nDummy = 0;
		xnOSStrFormat(strLabel, sizeof(strLabel), &nDummy, "Skeleton - %d FPS", smd.FPS());
		glColor4f(1, 0, 0, 1);
		glRasterPos2i(10, 40);
		glPrintString(GLUT_BITMAP_HELVETICA_18, strLabel);
	}
	else
	{
		nLastUserTime = 0;
	}

	//printf("USER FPS:%d  FrameId:%d  Timestamp:%d(passed time in microsecond)\n", smd.FPS(), smd.FrameID(), smd.Timestamp() );
	for (int i = 0; i < nUsers; ++i)
	{
#ifndef USE_GLES
		if (g_bPrintID)
		{
			XnPoint3D com;
			g_UserGenerator.GetCoM(aUsers[i], com);
			g_DepthGenerator.ConvertRealWorldToProjective(1, &com, &com);

			XnUInt32 nDummy = 0;

			xnOSMemSet(strLabel, 0, sizeof(strLabel));
			if (!g_bPrintState)
			{
				// Tracking
				xnOSStrFormat(strLabel, sizeof(strLabel), &nDummy, "%d", aUsers[i]);
			}
			else if (g_UserGenerator.GetSkeletonCap().IsTracking(aUsers[i]))
			{
				// Tracking
				xnOSStrFormat(strLabel, sizeof(strLabel), &nDummy, "%d - Tracking", aUsers[i]);
			}
			else if (g_UserGenerator.GetSkeletonCap().IsCalibrating(aUsers[i]))
			{
				// Calibrating
				xnOSStrFormat(strLabel, sizeof(strLabel), &nDummy, "%d - Calibrating [%s]", aUsers[i], GetCalibrationErrorString(m_Errors[aUsers[i]].first));
			}
			else
			{
				// Nothing
				xnOSStrFormat(strLabel, sizeof(strLabel), &nDummy, "%d - Looking for pose [%s]", aUsers[i], GetPoseErrorString(m_Errors[aUsers[i]].second));
			}

			glColor4f(1-Colors[i%nColors][0], 1-Colors[i%nColors][1], 1-Colors[i%nColors][2], 1);

			//glRasterPos2i(com.X, com.Y);
			glRasterPos2i(com.X/2, com.Y);
			glPrintString(GLUT_BITMAP_HELVETICA_18, strLabel);
		}
#endif
		// スケルトンデータ送信 & ファイル保存
		if (g_UserGenerator.GetSkeletonCap().IsTracking(aUsers[i]))
		{
//			printf("User:%d is Ttacking.. time:%ld\n", aUsers[i], smd.Timestamp());
			bool flag = true;
			// 配列番号5から格納開始
			int pos = SRV_SEND_HDR_NUM;
			for(int sCnt = 0; g_SendSkelJoints[sCnt] != -1; ++sCnt)
			{
				if(SetJointData(aUsers[i], g_SendSkelJoints[sCnt], &values[pos]) == false)
				{
					flag = false;
					break;
				}
				// X,Y,Zを格納したので3進める
				pos += 3;
			}

			if( flag == true)
			{
				skelCnt++;
				values[1] = aUsers[i];
				values[2] = aUsers[i];
				values[3] = skelCnt;

				// スケルトンデータ処理
				g_SkelMng.Update((const char*)time_stamp, (const float*)values);
			}
		}

		// 骨格描画
		if (g_bDrawSkeleton && g_UserGenerator.GetSkeletonCap().IsTracking(aUsers[i]))
		{
			glColor4f(1-Colors[aUsers[i]%nColors][0], 1-Colors[aUsers[i]%nColors][1], 1-Colors[aUsers[i]%nColors][2], 1);

			// Draw Joints
			if (g_bMarkJoints)
			{
				// Try to draw all joints
				DrawJoint(aUsers[i], XN_SKEL_HEAD);
				DrawJoint(aUsers[i], XN_SKEL_NECK);
				DrawJoint(aUsers[i], XN_SKEL_TORSO);
				DrawJoint(aUsers[i], XN_SKEL_WAIST);

				DrawJoint(aUsers[i], XN_SKEL_LEFT_COLLAR);
				DrawJoint(aUsers[i], XN_SKEL_LEFT_SHOULDER);
				DrawJoint(aUsers[i], XN_SKEL_LEFT_ELBOW);
				DrawJoint(aUsers[i], XN_SKEL_LEFT_WRIST);
				DrawJoint(aUsers[i], XN_SKEL_LEFT_HAND);
				DrawJoint(aUsers[i], XN_SKEL_LEFT_FINGERTIP);

				DrawJoint(aUsers[i], XN_SKEL_RIGHT_COLLAR);
				DrawJoint(aUsers[i], XN_SKEL_RIGHT_SHOULDER);
				DrawJoint(aUsers[i], XN_SKEL_RIGHT_ELBOW);
				DrawJoint(aUsers[i], XN_SKEL_RIGHT_WRIST);
				DrawJoint(aUsers[i], XN_SKEL_RIGHT_HAND);
				DrawJoint(aUsers[i], XN_SKEL_RIGHT_FINGERTIP);

				DrawJoint(aUsers[i], XN_SKEL_LEFT_HIP);
				DrawJoint(aUsers[i], XN_SKEL_LEFT_KNEE);
				DrawJoint(aUsers[i], XN_SKEL_LEFT_ANKLE);
				DrawJoint(aUsers[i], XN_SKEL_LEFT_FOOT);

				DrawJoint(aUsers[i], XN_SKEL_RIGHT_HIP);
				DrawJoint(aUsers[i], XN_SKEL_RIGHT_KNEE);
				DrawJoint(aUsers[i], XN_SKEL_RIGHT_ANKLE);
				DrawJoint(aUsers[i], XN_SKEL_RIGHT_FOOT);
			}

#ifndef USE_GLES
			glBegin(GL_LINES);
#endif
			// Draw Limbs
			DrawLimb(aUsers[i], XN_SKEL_HEAD, XN_SKEL_NECK);

			DrawLimb(aUsers[i], XN_SKEL_NECK, XN_SKEL_LEFT_SHOULDER);
			DrawLimb(aUsers[i], XN_SKEL_LEFT_SHOULDER, XN_SKEL_LEFT_ELBOW);
			if (!DrawLimb(aUsers[i], XN_SKEL_LEFT_ELBOW, XN_SKEL_LEFT_WRIST))
			{
				DrawLimb(aUsers[i], XN_SKEL_LEFT_ELBOW, XN_SKEL_LEFT_HAND);
			}
			else
			{
				DrawLimb(aUsers[i], XN_SKEL_LEFT_WRIST, XN_SKEL_LEFT_HAND);
				DrawLimb(aUsers[i], XN_SKEL_LEFT_HAND, XN_SKEL_LEFT_FINGERTIP);
			}


			DrawLimb(aUsers[i], XN_SKEL_NECK, XN_SKEL_RIGHT_SHOULDER);
			DrawLimb(aUsers[i], XN_SKEL_RIGHT_SHOULDER, XN_SKEL_RIGHT_ELBOW);
			if (!DrawLimb(aUsers[i], XN_SKEL_RIGHT_ELBOW, XN_SKEL_RIGHT_WRIST))
			{
				DrawLimb(aUsers[i], XN_SKEL_RIGHT_ELBOW, XN_SKEL_RIGHT_HAND);
			}
			else
			{
				DrawLimb(aUsers[i], XN_SKEL_RIGHT_WRIST, XN_SKEL_RIGHT_HAND);
				DrawLimb(aUsers[i], XN_SKEL_RIGHT_HAND, XN_SKEL_RIGHT_FINGERTIP);
			}

			DrawLimb(aUsers[i], XN_SKEL_LEFT_SHOULDER, XN_SKEL_TORSO);
			DrawLimb(aUsers[i], XN_SKEL_RIGHT_SHOULDER, XN_SKEL_TORSO);

			DrawLimb(aUsers[i], XN_SKEL_TORSO, XN_SKEL_LEFT_HIP);
			DrawLimb(aUsers[i], XN_SKEL_LEFT_HIP, XN_SKEL_LEFT_KNEE);
			DrawLimb(aUsers[i], XN_SKEL_LEFT_KNEE, XN_SKEL_LEFT_FOOT);

			DrawLimb(aUsers[i], XN_SKEL_TORSO, XN_SKEL_RIGHT_HIP);
			DrawLimb(aUsers[i], XN_SKEL_RIGHT_HIP, XN_SKEL_RIGHT_KNEE);
			DrawLimb(aUsers[i], XN_SKEL_RIGHT_KNEE, XN_SKEL_RIGHT_FOOT);

			DrawLimb(aUsers[i], XN_SKEL_LEFT_HIP, XN_SKEL_RIGHT_HIP);
#ifndef USE_GLES
			glEnd();
#endif
		}
	}

	if (g_bPrintFrameID)
	{
		// Depth
		static XnChar strFrameIDd[80];
		xnOSMemSet(strFrameIDd, 0, 80);
		XnUInt32 nDummy = 0;
		xnOSStrFormat(strFrameIDd, sizeof(strFrameIDd), &nDummy, "%d", dmd.FrameID());

		glColor4f(1, 0, 0, 1);

		glRasterPos2i(10, 20);

		glPrintString(GLUT_BITMAP_HELVETICA_18, strFrameIDd);

		// Image
		static XnChar strFrameIDi[80];
		xnOSMemSet(strFrameIDi, 0, 80);
		xnOSStrFormat(strFrameIDi, sizeof(strFrameIDi), &nDummy, "%d", imd.FrameID());

		glColor4f(1, 0, 0, 1);

		glRasterPos2i(330, 20);

		glPrintString(GLUT_BITMAP_HELVETICA_18, strFrameIDi);
	}
}

