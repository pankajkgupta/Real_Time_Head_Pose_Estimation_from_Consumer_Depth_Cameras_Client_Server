#include <XnOpenNI.h>
#include <XnCodecIDs.h>
#include <XnCppWrapper.h>
#include "SceneDrawer.h"
#include <XnPropNames.h>
#include <process.h>

#include "Common.h"
#include "Config.h"
#include "MovieMng.h"


#define LOG_WRITE_ENABLE
#ifdef LOG_WRITE_ENABLE
	#define LOG_NAME	"log_MovieMng.txt"
	FILE *mmng_fp = NULL;
#endif

#define FRAME_BUF_LENGTH		10		// 深度/イメージフレーム蓄積バッファ長(xx秒分)
#define DEPTH_SEND_TIMES		2		// 何回に1回送信するか

volatile XnBool MovieMng::m_bEnd = FALSE;

#ifdef _DEBUG
    //Debugモードの場合
    #pragma comment(lib,"C:\\opencv\\build\\x64\\vc10\\lib\\opencv_core241d.lib")
    #pragma comment(lib,"C:\\opencv\\build\\x64\\vc10\\lib\\opencv_imgproc241d.lib")
    #pragma comment(lib,"C:\\opencv\\build\\x64\\vc10\\lib\\opencv_highgui241d.lib")
#else
    //Releaseモードの場合
    #pragma comment(lib,"C:\\opencv\\build\\x64\\vc10\\lib\\opencv_core241.lib")
    #pragma comment(lib,"C:\\opencv\\build\\x64\\vc10\\lib\\opencv_imgproc241.lib")
    #pragma comment(lib,"C:\\opencv\\build\\x64\\vc10\\lib\\opencv_highgui241.lib")
#endif

// Creation - set the OpenNI objects
MovieMng::MovieMng(xn::Context& context, xn::DepthGenerator& depthGenerator, xn::ImageGenerator& imageGenerator) :
	m_context(context),
	m_depthGenerator(depthGenerator),
	m_imageGenerator(imageGenerator)
{
	m_bDepth = TRUE;
	m_bImage = TRUE;
	m_nNextWrite = 0;
	m_nWritten = m_nNextWrite;
	m_nSent = m_nNextWrite;
	m_nBufferSize = 0;
	m_nBufferSizeFile = 0;
	m_nBufferSizeSend = 0;
	m_nDataCountFile = 0;
	m_nDataCountSend = 0;
	m_FileWriterTh = NULL;
	m_DepthSenderTh = NULL;
	m_bFileWrite = FALSE;
	m_bSendDepth = FALSE;
	m_bFaceServerConnected = FALSE;
	m_bFaceServerInit = FALSE;
	m_fOpenTimestamp = 0;
}

// Initialization - set outdir and time of each recording
void MovieMng::Initialize(void)
{
#ifdef LOG_WRITE_ENABLE
	fopen_s(&mmng_fp, LOG_NAME, "a+");
	char time_str[TIME_STR_LEN];
	g_util.GetTimeStr(sizeof(time_str), time_str);
	fprintf(mmng_fp, "=== %s ===\n", time_str);
#endif
	// コンフィグファイル読み込み
	char tmp[16];
	Config config(CONFIG_FILE);
	config.Initialize();

	config.GetValue("MOVIE_LENGTH", sizeof(tmp), tmp);
	m_nMovieLength = atoi(tmp);

	config.GetValue("MOVIE_FILE_WRITE", sizeof(tmp), tmp);
	if(strcmp(tmp, "TRUE") == 0)
	{
		m_bFileWrite = TRUE;
	}

	config.GetValue("DATA_DIR", sizeof(m_strDirName), m_strDirName);
	config.GetValue("MOVIE_FILE_DIR", sizeof(m_strDirName), &m_strDirName[strlen(m_strDirName)]);

	config.GetValue("FACE_DIRECTION", sizeof(tmp), tmp);
	if(strcmp(tmp, "TRUE") == 0)
	{
		m_bSendDepth = TRUE;
	}

	config.GetValue("FACE_HOST", sizeof(m_faceHost), m_faceHost);
	
	config.GetValue("FACE_PORT", sizeof(tmp), tmp);
	m_facePort = (short)atoi(tmp);

	config.GetValue("MOVIE_LENGTH", sizeof(tmp), tmp);
	m_nFileLength = (short)atoi(tmp);

	// バッファ生成 バッファサイズ：10秒分(30fpsと仮定)
	m_nBufferSize = m_nBufferSizeFile = m_nBufferSizeSend = FRAME_BUF_LENGTH * 30;
	m_pFrames = XN_NEW_ARR(SingleFrame, m_nBufferSize);
	
	// スレッド生成
	if (m_bFileWrite == TRUE)
	{
		// ファイル書き込み用
		InitializeCriticalSection(&csFileWriter);
		m_FileWriterTh = (HANDLE)_beginthreadex( NULL, 0, &FileWriter, this, 0, &m_FileWriterId );
	}

	if (m_bSendDepth == TRUE)
	{
		ConnectFaceServer();
		// 深度情報送信用
		InitializeCriticalSection(&csDepthSender);
		m_DepthSenderTh = (HANDLE)_beginthreadex( NULL, 0, &DepthSender, this, 0, &m_DepthSenderId );
	}
}

void MovieMng::End(void)
{
	m_bEnd = TRUE;
	
	if (m_bSendDepth)
	{
		WaitForSingleObject(m_DepthSenderTh, 30 * 1000);
	}

	if (m_bFileWrite)
	{
		WaitForSingleObject(m_FileWriterTh, 30 * 1000);
	}

#ifdef LOG_WRITE_ENABLE
	fclose(mmng_fp);
#endif
}

// Save new data from OpenNI
void MovieMng::Update(const xn::DepthMetaData& dmd, const xn::ImageMetaData& imd)
{
	// バッファ空き確認
	if((m_nBufferSizeFile == 0) || (m_nBufferSizeSend == 0))
	{
		printf("バッファ空きなし！！ ForFile:%d ForSend:%d\n", m_nBufferSizeFile, m_nBufferSizeSend);
		return;
	}

	SingleFrame* pFrames = m_pFrames + m_nNextWrite;

	if(m_nNextWrite == 0)
	{
		m_nImageHeight = imd.FullYRes();
		m_nImageWidth = imd.FullXRes();
		m_nDepthHeight = dmd.FullYRes();
		m_nDepthWidth = dmd.FullXRes();

		printf("Image Height:%d Width:%d  Depth Height:%d Width:%d\n", 
			m_nImageHeight, m_nImageWidth, m_nDepthHeight, m_nDepthWidth);
	}
#ifdef LOG_WRITE_ENABLE
	fprintf(mmng_fp, "[%s] update. m_nNextWrite:%d\n", __FUNCTION__, m_nNextWrite);
#endif
	if (m_bDepth)
	{
		pFrames->depthFrame.CopyFrom(dmd);
	}
	
	if (m_bImage)
	{
		pFrames->imageFrame.CopyFrom(imd);
	}

	// バッファ管理情報書き換え
	if (m_bFileWrite)
	{
		EnterCriticalSection(&csFileWriter);
		m_nDataCountFile++;
		m_nBufferSizeFile--;
	}

	if (m_bSendDepth)
	{
		EnterCriticalSection(&csDepthSender);
		m_nDataCountSend++;
		m_nBufferSizeSend--;
	}
	
	m_nNextWrite++;
	if (m_nBufferSize == m_nNextWrite)
	{
		m_nNextWrite = 0;
	}

	if (m_bFileWrite)
	{
		LeaveCriticalSection(&csFileWriter);
	}
	
	if (m_bSendDepth)
	{
		LeaveCriticalSection(&csDepthSender);
	}
}

unsigned __stdcall MovieMng::FileWriter(void *p)
{
	printf("FileWriter START!!\n");

	MovieMng* mmngp = reinterpret_cast<MovieMng*>(p);
	
	while(!(m_bEnd && (mmngp->m_nDataCountFile == 0)))
	{
		XnUInt32 writeNum = mmngp->m_nDataCountFile;

		if(writeNum == 0)
		{
#ifdef _MSC_VER
			Sleep(30);
#else
#endif
			continue;
		}

#ifdef LOG_WRITE_ENABLE
		fprintf(mmng_fp, "[%s]WRITE NUM:%d\n", __FUNCTION__, writeNum);
#endif

		while(writeNum)
		{
			mmngp->DumpMovie();
			writeNum--;

			mmngp->m_nWritten++;
			if(mmngp->m_nBufferSize <= mmngp->m_nWritten)
			{
				mmngp->m_nWritten = 0;
			}

			EnterCriticalSection(&mmngp->csFileWriter);
			mmngp->m_nDataCountFile--;
			mmngp->m_nBufferSizeFile++;
			LeaveCriticalSection(&mmngp->csFileWriter);
		}
	}

	printf("FileWriter END...\n");
	_endthreadex( 0 );
    return 0;
}

void MovieMng::AddDummyFrame(VideoWriter& writer, XnUInt32 num)
{
	static int kind = 0;
	int value[3][3] = {(100,0,0), (0,100,0), (0,0,100)};
	for(XnUInt32 i = 0; i < num; i++)
	{
//		Mat image(m_nImageHeight, m_nImageWidth, CV_8UC3);
		Mat image(Size(m_nImageWidth, m_nImageHeight), CV_8UC3, 
				cv::Scalar(value[kind][0],value[kind][1],value[kind][2]));
		kind++;
		if (kind >= 3)
		{
			kind = 0;
		}
		cvtColor(image, image, CV_BGR2RGB);
//		imshow("luck", image);
		writer << image;
	}
}

// Save the current state of the buffer to a file
XnStatus MovieMng::DumpMovie(void)
{
	static VideoWriter writer_image;
	static VideoWriter writer_depth;

	Mat image(m_nImageHeight, m_nImageWidth, CV_8UC3);
	Mat depth(m_nDepthHeight, m_nDepthWidth, CV_16UC1);
	Mat detect(m_nDepthHeight, m_nDepthWidth, CV_16UC1);
	Mat detect8UC1(m_nDepthHeight, m_nDepthWidth, CV_8UC1);
	Mat detect8UC3(m_nDepthHeight, m_nDepthWidth, CV_8UC3);

	static double scale = 255.0/65535.0;

	static XnUInt64 nLastDepthTime = 0;
	static XnUInt64 nLastImageTime = 0;
	static XnUInt32 nMissedDepthFrames = 0;
	static XnUInt32 nMissedImageFrames = 0;
	static XnUInt32 nDepthFrames = 0;
	static XnUInt32 nImageFrames = 0;

	SingleFrame* pFrames = m_pFrames + m_nWritten;

	// ファイル変更チェック
	XnUInt64 timeStamp = pFrames->depthFrame.Timestamp();
	if (Util::GetTimeDiff(timeStamp, m_fOpenTimestamp) > (m_nMovieLength * 60 * 1000))
	{
		m_fOpenTimestamp = timeStamp;
		writer_image.release();
	}
	
	// ファイルオープン
	if(!writer_image.isOpened())
	{
		char timeStr[TIME_STR_LEN];
		g_util.GetTimeStr(pFrames->depthFrame.Timestamp(), sizeof(timeStr), timeStr);
		Util::ChangeIntoNum(timeStr);
		
		// ファイル名作成
		XnChar strFileName[XN_FILE_MAX_PATH];
	
		if (m_bDepth)
		{
			sprintf(strFileName, "%s/%s/%s%s.avi", m_strDirName, "depth/movie", "depth_", timeStr);
			writer_depth.open(strFileName, CV_FOURCC('x','v','i','d'), 29.97, Size(m_nDepthWidth,m_nDepthHeight));
		}

		if (m_bImage)
		{
			sprintf(strFileName, "%s/%s/%s%s.avi", m_strDirName, "video/movie", "image_", timeStr);
			writer_image.open(strFileName, CV_FOURCC('x','v','i','d'), 29.97, Size(640,480));
		}
	}

	XnUInt64 nTimestamp;
	if (m_bDepth)
	{
#ifdef LOG_WRITE_ENABLE
		fprintf(mmng_fp, "[%s] Depth Write.  buffer:%d FrameId:%d Addr:0x%x\n", 
			__FUNCTION__, m_nWritten, pFrames->depthFrame.FrameID(), pFrames);
#endif
		// フレーム落ちチェック
		nTimestamp = pFrames->depthFrame.Timestamp();
		int missed = Util::CheckMissedFrame("depth", nTimestamp, &nDepthFrames, &nLastDepthTime, &nMissedDepthFrames);
		AddDummyFrame(writer_depth, missed);
		
		memcpy(depth.data, pFrames->depthFrame.Data(), depth.step*depth.rows);
		detect = depth.clone();

		detect.convertTo(detect8UC1, CV_8UC1, scale, 0.0);	//16UC1to8UC1 画像として保存する場合に必要
		cvtColor(detect8UC1, detect8UC3, CV_GRAY2RGB);		//8UC1to8UC3 動画として保存する場合に必要

//		imshow("depth", depth);
		writer_depth<<detect8UC3;
	}

	if (m_bImage)
	{
#ifdef LOG_WRITE_ENABLE
		fprintf(mmng_fp, "[%s] Image Write.  buffer:%d FrameId:%d Addr:0x%x\n", 
			__FUNCTION__, m_nWritten, pFrames->imageFrame.FrameID(), pFrames);
#endif
		// フレーム落ちチェック
		nTimestamp = pFrames->imageFrame.Timestamp();
		int missed = Util::CheckMissedFrame("image", nTimestamp, &nImageFrames, &nLastImageTime, &nMissedImageFrames);
		AddDummyFrame(writer_image, missed);

		memcpy(image.data, pFrames->imageFrame.Data(), image.step*image.rows);
		cvtColor(image, image, CV_BGR2RGB);
//		imshow("image", image);
		writer_image << image;
	}

	return XN_STATUS_OK;
}

unsigned __stdcall MovieMng::DepthSender(void *p)
//unsigned __stdcall DepthSender(void* pArguments)
{
	printf("DepthSender START!!\n");

	MovieMng* mmngp = reinterpret_cast<MovieMng*>(p);

	while(!(m_bEnd && (mmngp->m_nDataCountSend == 0)))
	{
		XnUInt32 sendNum = mmngp->m_nDataCountSend;

		// 30ミリ秒wait
		if(sendNum == 0)
		{
#ifdef _MSC_VER
			Sleep(30);
#else
#endif
			continue;
		}
#ifdef LOG_WRITE_ENABLE
		fprintf(mmng_fp, "[%s]SEND NUM:%d\n", __FUNCTION__, sendNum);
#endif

		// 深度データ送信
		while(sendNum)
		{
			if (!(mmngp->m_nSent % DEPTH_SEND_TIMES))
			{
				mmngp->SendDepth();
			}
			sendNum--;

			mmngp->m_nSent++;
			if(mmngp->m_nBufferSize <= mmngp->m_nSent)
			{
				mmngp->m_nSent = 0;
			}
			
			EnterCriticalSection(&mmngp->csDepthSender);
			mmngp->m_nDataCountSend--;
			mmngp->m_nBufferSizeSend++;
			LeaveCriticalSection(&mmngp->csDepthSender);
		}
	}

	printf("DepthSender END...\n");
	_endthreadex( 0 );
    return 0;
}

void MovieMng::ConnectFaceServer(void)
{
	if (m_bFaceServerConnected == TRUE)
	{
		return;
	}

	// 顔認識機能用サーバ接続
	if(srvCom.InitServer(m_faceHost, m_facePort) == 0)
	{
		m_bFaceServerConnected = TRUE;
		m_bFaceServerInit = FALSE;
	}
}

void MovieMng::SendDepth(void)
{
	SingleFrame* pFrames = m_pFrames + m_nSent;
	if (m_bFaceServerConnected == FALSE)
	{
#ifdef LOG_WRITE_ENABLE
	fprintf(mmng_fp, "[%s] Depth Not Send(Server Not Connected.).  buffer:%d FrameID:%d ADDR:0x%p\n", 
		__FUNCTION__, m_nSent, pFrames->depthFrame.FrameID(), pFrames);
#endif
		return;
	}
	
#ifdef LOG_WRITE_ENABLE
	fprintf(mmng_fp, "[%s] Depth Send.  buffer:%d FrameID:%d ADDR:0x%p\n", 
		__FUNCTION__, m_nSent, pFrames->depthFrame.FrameID(), pFrames);
#endif

	// データコピー
	const XnDepthPixel* pDepth = pFrames->depthFrame.Data();
	XnUInt32 dataSize = pFrames->depthFrame.DataSize();

	XnDepthPixel *datap = new XnDepthPixel[dataSize];
	memcpy(datap, pDepth, dataSize);

	if (m_bFaceServerInit == FALSE)
	{
		// 送信データサイズ送信
		srvCom.SendData(sizeof(dataSize), (const char*)&dataSize);
		XnUInt64 g_focal_length;
		XnDouble g_pixel_size;
		
		// get the focal length in mm (ZPS = zero plane distance)
		m_depthGenerator.GetIntProperty ("ZPD", g_focal_length);
		srvCom.SendData(sizeof(g_focal_length), (const char*)&g_focal_length);
		
		// get the pixel size in mm ("ZPPS" = pixel size at zero plane)
		m_depthGenerator.GetRealProperty ("ZPPS", g_pixel_size);
		printf("g_pixel_size:%f\n", g_pixel_size);
		g_pixel_size *= 2.f;
		char tmp[8];
		memset(tmp, NULL, sizeof(tmp));
		sprintf_s(tmp, 8, "%02.04f", g_pixel_size);
		printf("tmp:%s\n", tmp);
		srvCom.SendData(7, (const char*)tmp);
		printf("g_pixel_size:%f\n", g_pixel_size);

		m_bFaceServerInit = TRUE;
	}

	//send XRes()
//	XnUInt32 xres = pFrames->depthFrame.XRes();
//	XnUInt32 yres = pFrames->depthFrame.YRes();
//	srvCom.SendData(sizeof(xres), (const char*)&xres);
//	srvCom.SendData(sizeof(yres), (const char*)&yres);
	srvCom.SendData(sizeof(m_nDepthWidth), (const char*)&m_nDepthWidth);
	srvCom.SendData(sizeof(m_nDepthHeight), (const char*)&m_nDepthHeight);
	
	// データ送信
	srvCom.SendData(dataSize, (const char*)datap);

	delete [] datap;
}
