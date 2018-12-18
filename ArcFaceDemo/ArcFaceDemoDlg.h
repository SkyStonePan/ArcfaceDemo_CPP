
// ArcFaceDemoDlg.h : 头文件
//

#pragma once
#include "afxcmn.h"
#include "ArcFaceEngine.h"

#include <memory>
#include <string>

//调用GDI+
#include <GdiPlus.h>
#include "afxwin.h"
#pragma comment(lib, "Gdiplus.lib")
using namespace Gdiplus;


// CArcFaceDemoDlg 对话框
class CArcFaceDemoDlg : public CDialogEx
{
	// 构造
public:
	CArcFaceDemoDlg(CWnd* pParent = NULL);	// 标准构造函数
	~CArcFaceDemoDlg();
	// 对话框数据
	enum { IDD = IDD_ARCFACEDEMO_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


	// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnBnClickedBtnRegister();
	afx_msg void OnBnClickedBtnRecognition();
	afx_msg void OnBnClickedBtnCompare();
	afx_msg void OnBnClickedBtnClear();
	afx_msg void OnDestroy();
	afx_msg void OnBnClickedBtnCamera();
	afx_msg void OnEnChangeEditThreshold();

	void EditOut(CString str, bool add_endl);
	void IplDrawToHDC(BOOL isVideoMode, IplImage* image, CRect& showRect,  UINT ID);

private:
	void LoadThumbnailImages();
	CString SelectFolder();
	BOOL TerminateLoadThread();
	BOOL ClearRegisterImages();
	BOOL CalculateShowPositon(IplImage*curSelectImage, Gdiplus::Rect& showRect);
	MRESULT StaticImageFaceOp(IplImage* image);
    void ClearShowWindow();
public:
	CListCtrl m_ImageListCtrl;
	CImageList m_IconImageList;
	CEdit m_editLog;

	CString m_folderPath;
	std::vector<ASF_FaceFeature> m_featuresVec;

	BOOL m_bLoadIconThreadRunning;
	DWORD m_dwLoadIconThreadID;
	HANDLE m_hLoadIconThread;

	BOOL m_bClearFeatureThreadRunning;
	DWORD m_dwClearFeatureThreadID;
	HANDLE m_hClearFeatureThread;

    BOOL m_bFDThreadRunning;
    DWORD m_dwFDThreadID;
    HANDLE m_hFDThread;

    BOOL m_bFRThreadRunning;
    DWORD m_dwFRThreadID;
    HANDLE m_hFRThread;

	ArcFaceEngine m_imageFaceEngine;
	ArcFaceEngine m_videoFaceEngine;

	IplImage* m_curStaticImage;//当前选中的图片
	ASF_FaceFeature m_curStaticImageFeature;//当前图片的人脸特征
	BOOL m_curStaticImageFRSucceed;
	Gdiplus::Rect m_curFaceShowRect;
	Gdiplus::Rect m_curImageShowRect;

	CString m_curStaticShowAgeGenderString;

	CString m_curStaticShowCmpString;

	//Bitmap* m_pSelectedImage;//待绘的图

	IplImage* m_curVideoImage;
	ASF_SingleFaceInfo m_curFaceInfo;
	bool m_dataValid;

	//cv::VideoCapture capture;
	bool m_videoOpened;
	CString m_strEditThreshold;

	Gdiplus::PointF m_curStringShowPosition;//当前字符串显示的位置
	CString m_curVideoShowString;

private:
	CRect m_windowViewRect;//展示控件的尺寸
	//IplImage* m_curSelectedImage;
	//std::shared_ptr<ASF_SingleFaceInfo> m_curDetectFaceInfo;

public:
	afx_msg void OnClose();
};
