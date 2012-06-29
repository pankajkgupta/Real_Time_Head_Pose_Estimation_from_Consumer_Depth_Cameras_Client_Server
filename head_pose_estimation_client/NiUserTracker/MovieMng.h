#ifndef MOVIE_MNG_H_
#define MOVIE_MNG_H_

#include <XnOpenNI.h>
#include <XnCodecIDs.h>
#include <XnCppWrapper.h>
#include "SceneDrawer.h"
#include <XnPropNames.h>
#include<opencv2/opencv.hpp>

#include "Common.h"
#include "ServerCom.h"

using namespace cv;

// The cyclic buffer, to which frames will be added and from where they will be dumped to files
class MovieMng
{
public:
	// Creation - set the OpenNI objects
	MovieMng(xn::Context& context, xn::DepthGenerator& depthGenerator, xn::ImageGenerator& imageGenerator);

	// Initialization - set outdir and time of each recording
	void Initialize(void);

	void End(void);

	// Save new data from OpenNI
	void Update(const xn::DepthMetaData& dmd, const xn::ImageMetaData& imd);

	//
	void ConnectFaceServer();

protected:
	struct SingleFrame
	{
		xn::DepthMetaData depthFrame;
		xn::ImageMetaData imageFrame;
	};

	XnBool m_bDepth, m_bImage;
	XnBool m_bFileWrite;
	XnBool m_bSendDepth;
	XnBool m_bFaceServerConnected;
	XnBool m_bFaceServerInit;
	static volatile XnBool m_bEnd;
	SingleFrame* m_pFrames;
	XnUInt32 m_nNextWrite;
	XnUInt32 m_nWritten;
	XnUInt32 m_nSent;
	XnUInt32 m_nBufferSize;
	XnUInt32 m_nBufferSizeFile;
	XnUInt32 m_nBufferSizeSend;
	XnUInt32 m_nDataCountFile;
	XnUInt32 m_nDataCountSend;
	XnChar m_strDirName[XN_FILE_MAX_PATH];
	XnChar m_faceHost[16];
	XnUInt16 m_facePort;
	int m_nMovieLength;
	int m_nFileLength;
	XnUInt32 m_nImageHeight;
	XnUInt32 m_nImageWidth;
	XnUInt32 m_nDepthHeight;
	XnUInt32 m_nDepthWidth;
	XnUInt64 m_fOpenTimestamp;
	
	xn::Context& m_context;
	xn::DepthGenerator& m_depthGenerator;
	xn::ImageGenerator& m_imageGenerator;
	xn::Recorder m_recorder;

#ifdef _MSC_VER
	HANDLE m_FileWriterTh;
	unsigned m_FileWriterId;
	CRITICAL_SECTION csFileWriter;

	HANDLE m_DepthSenderTh;
	unsigned m_DepthSenderId;
	CRITICAL_SECTION csDepthSender;
#endif

private:
	XN_DISABLE_COPY_AND_ASSIGN(MovieMng);

	static unsigned __stdcall MovieMng::FileWriter(void *p);
	static unsigned __stdcall MovieMng::DepthSender(void *p);

	XnStatus DumpMovie(void);
	void SendDepth(void);

	void AddDummyFrame(VideoWriter& writer, XnUInt32 num);


//	static volatile XnBool endFlag;
	ServerCom srvCom;
	Util g_util;
#ifdef LOG_WRITE_ENABLE
	FILE *log_fp = NULL;
#endif
};

#endif