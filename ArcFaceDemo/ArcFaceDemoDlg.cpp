
// ArcFaceDemoDlg.cpp : 实现文件
//

#include "stdafx.h"

#include "ArcFaceDemo.h"
#include "ArcFaceDemoDlg.h"
#include "afxdialogex.h"
#include <afx.h>
#include <vector>
#include "opencv/cv.h"
#include "opencv/highgui.h"
#include <mutex>


using namespace std;
using namespace Gdiplus;

#define VIDEO_FRAME_DEFAULT_WIDTH 640
#define VIDEO_FRAME_DEFAULT_HEIGHT 480

#define FACE_FEATURE_SIZE 1032

#define THUMBNAIL_WIDTH  55
#define THUMBNAIL_HEIGHT  55
#define Threshold 0.80

mutex g_mutex;

unsigned long _stdcall RunLoadThumbnailThread(LPVOID lpParam);
unsigned long _stdcall RunFaceFeatureOperation(LPVOID lpParam);
unsigned long _stdcall RunFaceDetectOperation(LPVOID lpParam);
unsigned long _stdcall ClearFaceFeatureOperation(LPVOID lpParam);
Bitmap* IplImage2Bitmap(const IplImage* pIplImg);
IplImage* Bitmap2IplImage(Bitmap* pBitmap);
CBitmap* IplImage2CBitmap(const IplImage *img);

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
    CAboutDlg();

    // 对话框数据
    enum { IDD = IDD_ABOUTBOX };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

    // 实现
protected:
    DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CArcFaceDemoDlg 对话框

CArcFaceDemoDlg::CArcFaceDemoDlg(CWnd* pParent /*=NULL*/)
    : CDialogEx(CArcFaceDemoDlg::IDD, pParent),
    m_strEditThreshold(_T("")),
    m_curStaticImageFRSucceed(FALSE)
{
    m_hIcon = AfxGetApp()->LoadIcon(IDI_ICON_ARCSOFT);
}

CArcFaceDemoDlg::~CArcFaceDemoDlg()
{

}

void CArcFaceDemoDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST_IMAGE, m_ImageListCtrl);
    DDX_Control(pDX, IDC_EDIT_LOG, m_editLog);
    DDX_Text(pDX, IDC_EDIT_THRESHOLD, m_strEditThreshold);
}

BEGIN_MESSAGE_MAP(CArcFaceDemoDlg, CDialogEx)
    ON_WM_SYSCOMMAND()
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_BN_CLICKED(IDC_BTN_REGISTER, &CArcFaceDemoDlg::OnBnClickedBtnRegister)
    ON_BN_CLICKED(IDC_BTN_RECOGNITION, &CArcFaceDemoDlg::OnBnClickedBtnRecognition)
    ON_BN_CLICKED(IDC_BTN_COMPARE, &CArcFaceDemoDlg::OnBnClickedBtnCompare)
    ON_BN_CLICKED(IDC_BTN_CLEAR, &CArcFaceDemoDlg::OnBnClickedBtnClear)
    ON_WM_DESTROY()
    ON_BN_CLICKED(IDC_BTN_CAMERA, &CArcFaceDemoDlg::OnBnClickedBtnCamera)
    ON_EN_CHANGE(IDC_EDIT_THRESHOLD, &CArcFaceDemoDlg::OnEnChangeEditThreshold)
    ON_WM_CLOSE()
END_MESSAGE_MAP()


// CArcFaceDemoDlg 消息处理程序

BOOL CArcFaceDemoDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // 将“关于...”菜单项添加到系统菜单中。

    // IDM_ABOUTBOX 必须在系统命令范围内。
    ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
    ASSERT(IDM_ABOUTBOX < 0xF000);

    CMenu* pSysMenu = GetSystemMenu(FALSE);
    if (pSysMenu != NULL)
    {
        BOOL bNameValid;
        CString strAboutMenu;
        bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
        ASSERT(bNameValid);
        if (!strAboutMenu.IsEmpty())
        {
            pSysMenu->AppendMenu(MF_SEPARATOR);
            pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
        }
    }

    // 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
    //  执行此操作
    SetIcon(m_hIcon, TRUE);			// 设置大图标
    SetIcon(m_hIcon, FALSE);		// 设置小图标

    // TODO:  在此添加额外的初始化代码


    m_IconImageList.Create(THUMBNAIL_WIDTH,
        THUMBNAIL_HEIGHT,
        ILC_COLOR32,
        0,
        1);

    m_ImageListCtrl.SetImageList(&m_IconImageList, LVSIL_NORMAL);

    CString resStr;

    MRESULT faceRes = m_imageFaceEngine.ActiveSDK();
    resStr.Format("激活结果: %d\n", faceRes);
    EditOut(resStr, TRUE);

    faceRes = m_imageFaceEngine.InitEngine(ASF_DETECT_MODE_IMAGE);//Image
    resStr.Format("初始化结果: %d", faceRes);
    EditOut(resStr, TRUE);

    faceRes = m_videoFaceEngine.InitEngine(ASF_DETECT_MODE_VIDEO);//Video
    resStr.Format("初始化结果: %d", faceRes);
    EditOut(resStr, TRUE);


    //设置输入框位数
    ((CEdit*)GetDlgItem(IDC_EDIT_THRESHOLD))->SetLimitText(4);
    m_strEditThreshold.Format("%.2f", Threshold);
    UpdateData(FALSE);

    GetDlgItem(IDC_STATIC_VIEW)->GetWindowRect(&m_windowViewRect);

    //人脸库按钮置灰
    GetDlgItem(IDC_BTN_COMPARE)->EnableWindow(FALSE);
    GetDlgItem(IDC_BTN_CAMERA)->EnableWindow(FALSE);

    //编辑阈值置灰
    GetDlgItem(IDC_EDIT_THRESHOLD)->EnableWindow(FALSE);

    m_curStaticImageFeature.featureSize = FACE_FEATURE_SIZE;
    m_curStaticImageFeature.feature = (MByte *)malloc(m_curStaticImageFeature.featureSize * sizeof(MByte));


    return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CArcFaceDemoDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
    if ((nID & 0xFFF0) == IDM_ABOUTBOX)
    {
        CAboutDlg dlgAbout;
        dlgAbout.DoModal();
    }
    else
    {
        CDialogEx::OnSysCommand(nID, lParam);
    }
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。


void CArcFaceDemoDlg::OnPaint()
{
    if (IsIconic())
    {
        CPaintDC dc(this); // 用于绘制的设备上下文

        SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

        // 使图标在工作区矩形中居中
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;

        // 绘制图标
        dc.DrawIcon(x, y, m_hIcon);
    }
    else
    {
        if (m_videoOpened)
        {
            lock_guard<mutex> lock(g_mutex);
            //文字显示框
            CRect rect(m_curFaceInfo.faceRect.left - 10, m_curFaceInfo.faceRect.top - 50,
                m_curFaceInfo.faceRect.right, m_curFaceInfo.faceRect.bottom);
            IplDrawToHDC(TRUE, m_curVideoImage, rect, IDC_STATIC_VIEW);
        }
        else
        {
            if (m_curStaticImage)
            {
                CRect rect((int)m_curStringShowPosition.X + 10, (int)m_curStringShowPosition.Y + 10, 40, 40);
                IplDrawToHDC(FALSE, m_curStaticImage, rect, IDC_STATIC_VIEW);
            }
        }

        CDialogEx::OnPaint();
    }


}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CArcFaceDemoDlg::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(m_hIcon);
}

BOOL IsImageGDIPLUSValid(CString filePath)
{
    Bitmap image(filePath.AllocSysString());

    if (image.GetFlags() == ImageFlagsNone)
        return FALSE;
    else
        return TRUE;
}


//加载缩略图片
void CArcFaceDemoDlg::LoadThumbnailImages()
{
    m_bLoadIconThreadRunning = TRUE;

    m_hLoadIconThread = CreateThread(
        NULL,
        0,
        RunLoadThumbnailThread,
        this,
        0,
        &m_dwLoadIconThreadID);

    if (m_hLoadIconThread == NULL)
    {
        ::CloseHandle(m_hLoadIconThread);
    }
}


void CArcFaceDemoDlg::OnBnClickedBtnRegister()
{
    // TODO:  在此添加控件通知处理程序代码
    m_folderPath = SelectFolder();
    if (m_folderPath == "")
    {
        return;
    }
    LoadThumbnailImages();
}

unsigned long _stdcall RunLoadThumbnailThread(LPVOID lpParam)
{
    CArcFaceDemoDlg* dialog = (CArcFaceDemoDlg*)(lpParam);

    if (dialog == nullptr)
    {
        dialog->m_bLoadIconThreadRunning = FALSE;
        return 1;
    }

    if (dialog->m_folderPath == "")
    {
        dialog->m_bLoadIconThreadRunning = FALSE;
        return 1;
    }

    int iExistFeatureSize = (int)dialog->m_featuresVec.size();

    CString resStr;
    resStr.Format("开始注册人脸库");
    dialog->EditOut(resStr, TRUE);

    CFileFind finder;

    CString m_strCurrentDirectory(dialog->m_folderPath);
    CString strWildCard(m_strCurrentDirectory);
    vector<CString> m_vFileName;
    strWildCard += "\\*.*";

    BOOL bWorking = finder.FindFile(strWildCard);

    while (bWorking)
    {
        bWorking = finder.FindNextFile();

        if (finder.IsDots() || finder.IsDirectory())
        {
            continue;
        }

        CString filePath = finder.GetFileName();

        if (IsImageGDIPLUSValid(m_strCurrentDirectory + _T("\\") + filePath))//是否是图片
        {
            m_vFileName.push_back(filePath);
        }
    }

    resStr.Format("已选择图片张数: %d", m_vFileName.size());
    dialog->EditOut(resStr, TRUE);

    dialog->GetDlgItem(IDC_BTN_CLEAR)->EnableWindow(FALSE);
    dialog->GetDlgItem(IDC_BTN_REGISTER)->EnableWindow(FALSE);
    dialog->GetDlgItem(IDC_BTN_COMPARE)->EnableWindow(FALSE);

    if (dialog->GetDlgItem(IDC_BTN_RECOGNITION)->IsWindowEnabled())
    {
        dialog->GetDlgItem(IDC_BTN_RECOGNITION)->EnableWindow(FALSE);
    }

    vector<CString>::const_iterator iter;

    int actualIndex = iExistFeatureSize;

    for (iter = m_vFileName.begin();
        iter != m_vFileName.end();
        iter++)
    {
        if (!dialog->m_bLoadIconThreadRunning)
        {
            dialog->m_bLoadIconThreadRunning = FALSE;
            return 1;
        }

        CString imagePath;
        imagePath.Empty();
        imagePath.Format("%s\\%s", m_strCurrentDirectory, *iter);

        USES_CONVERSION;
        IplImage* originImage = cvLoadImage(T2A(imagePath.GetBuffer(0)));
        imagePath.ReleaseBuffer();

        if (!originImage)
        {
            cvReleaseImage(&originImage);
            continue;
        }

        //FD 
        ASF_SingleFaceInfo faceInfo = { 0 };
        MRESULT detectRes = dialog->m_imageFaceEngine.PreDetectFace(originImage, faceInfo);
        if (MOK != detectRes)
        {
            cvReleaseImage(&originImage);
            continue;
        }

        //FR
        ASF_FaceFeature faceFeature = { 0 };
        faceFeature.featureSize = FACE_FEATURE_SIZE;
        faceFeature.feature = (MByte *)malloc(faceFeature.featureSize * sizeof(MByte));
        detectRes = dialog->m_imageFaceEngine.PreExtractFeature(originImage, faceFeature, faceInfo);

        if (MOK != detectRes)
        {
            free(faceFeature.feature);
            cvReleaseImage(&originImage);
            continue;
        }

        Bitmap* image = IplImage2Bitmap(originImage);
        dialog->m_featuresVec.push_back(faceFeature);

        //计算缩略图显示位置
        int sourceWidth = image->GetWidth();
        int sourceHeight = image->GetHeight();

        int destX = 0;
        int destY = 0;

        float nPercent = 0;
        float nPercentW = ((float)THUMBNAIL_WIDTH / (float)sourceWidth);;
        float nPercentH = ((float)THUMBNAIL_HEIGHT / (float)sourceHeight);

        if (nPercentH < nPercentW)
        {
            nPercent = nPercentH;
            destX = (int)((THUMBNAIL_WIDTH - (sourceWidth * nPercent)) / 2);
        }
        else
        {
            nPercent = nPercentW;
            destY = (int)((THUMBNAIL_HEIGHT - (sourceHeight * nPercent)) / 2);
        }

        int destWidth = (int)(sourceWidth * nPercent);
        int destHeight = (int)(sourceHeight * nPercent);

        dialog->m_ImageListCtrl.InsertItem(actualIndex, to_string(actualIndex + 1).c_str(), actualIndex);

        actualIndex++;

        Bitmap* bmPhoto = new Bitmap(THUMBNAIL_WIDTH, THUMBNAIL_HEIGHT, PixelFormat24bppRGB);

        bmPhoto->SetResolution(image->GetHorizontalResolution(), image->GetVerticalResolution());

        Graphics *grPhoto = Graphics::FromImage(bmPhoto);
        Gdiplus::Color colorW(255, 255, 255, 255);
        grPhoto->Clear(colorW);
        grPhoto->SetInterpolationMode(InterpolationModeHighQualityBicubic);
        grPhoto->DrawImage(image, Gdiplus::Rect(destX, destY, destWidth, destHeight));

        HBITMAP hbmReturn = NULL;
        bmPhoto->GetHBITMAP(colorW, &hbmReturn);

        CBitmap Bmp1;
        Bmp1.Attach(hbmReturn);

        dialog->m_IconImageList.Add(&Bmp1, RGB(0, 0, 0));

        delete grPhoto;
        delete bmPhoto;
        Bmp1.Detach();
        DeleteObject(hbmReturn);

        dialog->m_ImageListCtrl.RedrawItems(actualIndex, actualIndex);

        //重绘
        if (actualIndex % 10 == 0)
        {
            dialog->m_ImageListCtrl.SetRedraw(TRUE);
            dialog->m_ImageListCtrl.Invalidate();
            dialog->m_ImageListCtrl.EnsureVisible(actualIndex - 1, FALSE);
        }

        cvReleaseImage(&originImage);
        delete image;
    }

    resStr.Format("成功注册图片张数: %d", actualIndex - iExistFeatureSize);
    dialog->EditOut(resStr, TRUE);

    dialog->m_ImageListCtrl.SetRedraw(TRUE);
    dialog->m_ImageListCtrl.Invalidate();
    dialog->m_ImageListCtrl.EnsureVisible(actualIndex - 1, FALSE);

    if (dialog->m_featuresVec.empty())
    {

    }

    //注册人脸库后按钮重置
    dialog->GetDlgItem(IDC_BTN_REGISTER)->EnableWindow(TRUE);

    if (!dialog->m_videoOpened)
    {
        dialog->GetDlgItem(IDC_BTN_COMPARE)->EnableWindow(TRUE);
        dialog->GetDlgItem(IDC_BTN_RECOGNITION)->EnableWindow(TRUE);
    }
    else
    {
        dialog->GetDlgItem(IDC_BTN_RECOGNITION)->EnableWindow(FALSE);
    }

    dialog->GetDlgItem(IDC_BTN_CAMERA)->EnableWindow(TRUE);
    dialog->GetDlgItem(IDC_BTN_REGISTER)->EnableWindow(TRUE);
    dialog->GetDlgItem(IDC_BTN_CLEAR)->EnableWindow(TRUE);

    dialog->m_bLoadIconThreadRunning = FALSE;

    return 0;
}

//选择文件夹
CString CArcFaceDemoDlg::SelectFolder()
{
    TCHAR           szFolderPath[MAX_PATH] = { 0 };
    CString         strFolderPath = TEXT("");

    BROWSEINFO      sInfo;
    ::ZeroMemory(&sInfo, sizeof(BROWSEINFO));
    sInfo.pidlRoot = 0;
    sInfo.lpszTitle = _T("请选择一个文件夹：");
    sInfo.ulFlags = BIF_DONTGOBELOWDOMAIN | BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE | BIF_EDITBOX;
    sInfo.lpfn = NULL;

    // 显示文件夹选择对话框  
    LPITEMIDLIST lpidlBrowse = ::SHBrowseForFolder(&sInfo);
    if (lpidlBrowse != NULL)
    {
        // 取得文件夹名  
        if (::SHGetPathFromIDList(lpidlBrowse, szFolderPath))
        {
            strFolderPath = szFolderPath;
        }
    }
    if (lpidlBrowse != NULL)
    {
        ::CoTaskMemFree(lpidlBrowse);
    }

    return strFolderPath;
}


void CArcFaceDemoDlg::OnBnClickedBtnRecognition()
{
    // TODO:  在此添加控件通知处理程序代码

    CFileDialog fileDlg(TRUE, _T("bmp"), NULL, 0, _T("Picture Files|*.jpg;*.jpeg;*.png;*.bmp;||"), NULL);
    fileDlg.DoModal();
    CString strFilePath;
    strFilePath = fileDlg.GetPathName();

    if (strFilePath == _T(""))
        return;

    USES_CONVERSION;
    IplImage* image = cvLoadImage(T2A(strFilePath.GetBuffer(0)));
    strFilePath.ReleaseBuffer();
    if (!image)
    {
        cvReleaseImage(&image);
        return;
    }

    if (m_curStaticImage)
    {
        cvReleaseImage(&m_curStaticImage);
        m_curStaticImage = NULL;
    }

    m_curStaticImage = cvCloneImage(image);
    cvReleaseImage(&image);

    StaticImageFaceOp(m_curStaticImage);

    GetDlgItem(IDC_BTN_COMPARE)->EnableWindow(TRUE);
}

MRESULT CArcFaceDemoDlg::StaticImageFaceOp(IplImage* image)
{
    Gdiplus::Rect showRect;
    CalculateShowPositon(image, showRect);
    m_curImageShowRect = showRect;
    //FD
    ASF_SingleFaceInfo faceInfo = { 0 };
    MRESULT detectRes = m_imageFaceEngine.PreDetectFace(image, faceInfo);

    //初始化
    m_curStaticShowAgeGenderString = "";
    m_curStaticShowCmpString = "";

    m_curFaceShowRect = Rect(0, 0, 0, 0);

    SendMessage(WM_PAINT);

    if (MOK == detectRes)
    {
        //show rect
        int n_top = showRect.Height*faceInfo.faceRect.top / image->height;
        int n_bottom = showRect.Height*faceInfo.faceRect.bottom / image->height;
        int n_left = showRect.Width*faceInfo.faceRect.left / image->width;
        int n_right = showRect.Width*faceInfo.faceRect.right / image->width;

        m_curFaceShowRect.X = n_left + showRect.X;
        m_curFaceShowRect.Y = n_top + showRect.Y;
        m_curFaceShowRect.Width = n_right - n_left;
        m_curFaceShowRect.Height = n_bottom - n_top;

        //显示文字在图片左上角
        m_curStringShowPosition.X = (REAL)(showRect.X);
        m_curStringShowPosition.Y = (REAL)(showRect.Y);

        //age gender
        ASF_MultiFaceInfo multiFaceInfo = { 0 };
        multiFaceInfo.faceOrient = (MInt32*)malloc(sizeof(MInt32));
        multiFaceInfo.faceRect = (MRECT*)malloc(sizeof(MRECT));

        multiFaceInfo.faceNum = 1;
        multiFaceInfo.faceOrient[0] = faceInfo.faceOrient;
        multiFaceInfo.faceRect[0] = faceInfo.faceRect;

        ASF_AgeInfo ageInfo = { 0 };
        ASF_GenderInfo genderInfo = { 0 };
        ASF_Face3DAngle angleInfo = { 0 };

        //age 、gender 、3d angle 信息
        detectRes = m_imageFaceEngine.FaceASFProcess(multiFaceInfo, image,
            ageInfo, genderInfo, angleInfo);

        if (MOK == detectRes)
        {
            CString showStr;
            showStr.Format("年龄:%d,性别:%s", ageInfo.ageArray[0], genderInfo.genderArray[0] == 0 ? "男" : "女");
            m_curStaticShowAgeGenderString = showStr;
        }
        else
        {
            m_curStaticShowAgeGenderString = "";
        }

        SendMessage(WM_PAINT);

        free(multiFaceInfo.faceRect);
        free(multiFaceInfo.faceOrient);

        //FR
        detectRes = m_imageFaceEngine.PreExtractFeature(image, m_curStaticImageFeature, faceInfo);

        if (MOK == detectRes)
        {
            m_curStaticImageFRSucceed = TRUE;
        }
        else//提取特征不成功
        {
            m_curStaticImageFRSucceed = FALSE;
            CString resStr;
            resStr.Format("特征提取失败");
            EditOut(resStr, TRUE);
            return -1;
        }
        return MOK;
    }
    else
    {
        m_curStaticImageFRSucceed = FALSE;

        CString resStr;
        resStr.Format("未检测到人脸");
        EditOut(resStr, TRUE);
        return -1;
    }
}

void CArcFaceDemoDlg::EditOut(CString str, bool add_endl)
{
    if (add_endl)
        str += "\r\n";
    int iLen = m_editLog.GetWindowTextLength();
    m_editLog.SetSel(iLen, iLen, TRUE);
    m_editLog.ReplaceSel(str, FALSE);
}


IplImage* Bitmap2IplImage(Bitmap* pBitmap)
{
    if (!pBitmap)
        return NULL;

    int w = pBitmap->GetWidth();
    int h = pBitmap->GetHeight();

    BitmapData bmpData;
    Gdiplus::Rect rect(0, 0, w, h);
    pBitmap->LockBits(&rect, ImageLockModeRead, PixelFormat24bppRGB, &bmpData);
    BYTE* temp = (bmpData.Stride > 0) ? ((BYTE*)bmpData.Scan0) : ((BYTE*)bmpData.Scan0 + bmpData.Stride*(h - 1));

    IplImage* pIplImg = cvCreateImage(cvSize(w, h), IPL_DEPTH_8U, 3);
    if (!pIplImg)
    {
        pBitmap->UnlockBits(&bmpData);
        return NULL;
    }

    memcpy(pIplImg->imageData, temp, abs(bmpData.Stride)*bmpData.Height);
    pBitmap->UnlockBits(&bmpData);

    //判断Top-Down or Bottom-Up
    if (bmpData.Stride < 0)
        cvFlip(pIplImg, pIplImg);

    return pIplImg;
}

// pBitmap 同样需要外部释放
Bitmap* IplImage2Bitmap(const IplImage* pIplImg)
{
    if (!pIplImg)
        return NULL;

    Bitmap *pBitmap = new Bitmap(pIplImg->width, pIplImg->height, PixelFormat24bppRGB);
    if (!pBitmap)
        return NULL;

    BitmapData bmpData;
    Gdiplus::Rect rect(0, 0, pIplImg->width, pIplImg->height);
    pBitmap->LockBits(&rect, ImageLockModeWrite, PixelFormat24bppRGB, &bmpData);
    //BYTE *pByte = (BYTE*)bmpData.Scan0;

    if (pIplImg->widthStep == bmpData.Stride) //likely
        memcpy(bmpData.Scan0, pIplImg->imageDataOrigin, pIplImg->imageSize);

    pBitmap->UnlockBits(&bmpData);
    return pBitmap;
}

void CArcFaceDemoDlg::OnBnClickedBtnCompare()
{
    // TODO:  在此添加控件通知处理程序代码

    if (!m_curStaticImageFRSucceed)
    {
        AfxMessageBox(_T("人脸比对失败，请重新选择识别照!"));
        return;
    }

    if (m_featuresVec.size() == 0)
    {
        AfxMessageBox(_T("还未选择注册图片！"));
        return;
    }

    int maxIndex = 1;//默认1号开始
    MFloat maxThreshold = 0.0;
    int curIndex = 0;

    //FR 比对
    for each (auto regisFeature in m_featuresVec)
    {
        curIndex++;
        MFloat confidenceLevel = 0;
        MRESULT pairRes = m_imageFaceEngine.FacePairMatching(confidenceLevel, m_curStaticImageFeature, regisFeature);

        if (MOK == pairRes &&confidenceLevel > maxThreshold)
        {
            maxThreshold = confidenceLevel;
            maxIndex = curIndex;
        }
    }

    CString resStr;

    //显示结果		
    resStr.Format("%d号:%.4f\n", maxIndex, maxThreshold);
    EditOut(resStr, TRUE);
    m_curStaticShowCmpString = resStr;
    SendMessage(WM_PAINT);

    resStr.Format("比对结束");
    EditOut(resStr, TRUE);
}


void CArcFaceDemoDlg::OnBnClickedBtnClear()
{
    // TODO: 在此添加控件通知处理程序代码

    if (m_videoOpened)
    {
        AfxMessageBox(_T("请先关闭摄像头！"));
        return;
    }

    // 注册人脸库按钮置灰
    GetDlgItem(IDC_BTN_COMPARE)->EnableWindow(FALSE);
    GetDlgItem(IDC_BTN_CAMERA)->EnableWindow(FALSE);
    GetDlgItem(IDC_BTN_REGISTER)->EnableWindow(FALSE);
    GetDlgItem(IDC_BTN_CLEAR)->EnableWindow(FALSE);

    //清理原有的图片以及特征
    ClearRegisterImages();

    GetDlgItem(IDC_BTN_REGISTER)->EnableWindow(TRUE);
    GetDlgItem(IDC_BTN_CAMERA)->EnableWindow(TRUE);

}

BOOL CArcFaceDemoDlg::TerminateLoadThread()
{
    while (m_bLoadIconThreadRunning)
    {
        MSG message;
        while (::PeekMessage(&message, NULL, 0, 0, PM_REMOVE))
        {
            ::TranslateMessage(&message);
            ::DispatchMessage(&message);
        }
    }

    ::CloseHandle(m_hLoadIconThread);

    return TRUE;
}

BOOL CArcFaceDemoDlg::ClearRegisterImages()
{
    if (m_bLoadIconThreadRunning)
    {
        TerminateLoadThread();
    }
    else
    {
        m_bClearFeatureThreadRunning = TRUE;

        m_hClearFeatureThread = CreateThread(
            NULL,
            0,
            ClearFaceFeatureOperation,//
            this,
            0,
            &m_dwClearFeatureThreadID);

        if (m_dwClearFeatureThreadID == NULL)
        {
            ::CloseHandle(m_hClearFeatureThread);
        }
    }
    return 0;
}

BOOL CArcFaceDemoDlg::CalculateShowPositon(IplImage*curSelectImage, Gdiplus::Rect& showRect)
{
    //计算实际显示宽高
    int actualWidth = 0;
    int actualHeight = 0;

    int imageWidth = curSelectImage->width;
    int imageHeight = curSelectImage->height;

    int windowWidth = m_windowViewRect.Width();
    int windowHeight = m_windowViewRect.Height();

    int paddingLeft = 0;
    int paddingTop = 0;

    //以宽为基准的高
    actualHeight = windowWidth*imageHeight / imageWidth;
    if (actualHeight > windowHeight)
    {
        //以高为基准的宽
        actualWidth = windowHeight*imageWidth / imageHeight;
        actualHeight = windowHeight;
    }
    else
    {
        actualWidth = windowWidth;
    }

    paddingLeft = (windowWidth - actualWidth) / 2;
    paddingTop = (windowHeight - actualHeight) / 2;

    showRect.X = paddingLeft;
    showRect.Y = paddingTop;
    showRect.Width = actualWidth;
    showRect.Height = actualHeight;

    return 0;
}


void CArcFaceDemoDlg::OnDestroy()
{
    CDialogEx::OnDestroy();
}


void CArcFaceDemoDlg::OnBnClickedBtnCamera()
{
    // TODO: 在此添加控件通知处理程序代码

    CString btnLabel;

    GetDlgItem(IDC_BTN_CAMERA)->GetWindowText(btnLabel);

    //防止太频繁点击按钮
    Sleep(1000);

    if (btnLabel == "启用摄像头")
    {

        GetDlgItem(IDC_EDIT_THRESHOLD)->EnableWindow(TRUE);
        GetDlgItem(IDC_BTN_COMPARE)->EnableWindow(FALSE);
        GetDlgItem(IDC_BTN_RECOGNITION)->EnableWindow(FALSE);
        GetDlgItem(IDC_BTN_CAMERA)->SetWindowText("关闭摄像头");

        //FD 线程
        m_hFDThread = CreateThread(
            NULL,
            0,
            RunFaceDetectOperation,
            this,
            0,
            &m_dwFDThreadID);

        if (m_hFDThread == NULL)
        {
            ::CloseHandle(m_hFDThread);
        }

        m_bFDThreadRunning = TRUE;

        //FR 线程
        m_hFRThread = CreateThread(
            NULL,
            0,
            RunFaceFeatureOperation,
            this,
            0,
            &m_dwFRThreadID);

        if (m_hFRThread == NULL)
        {
            ::CloseHandle(m_hFRThread);
        }


    }
    else
    {
        GetDlgItem(IDC_BTN_COMPARE)->EnableWindow(TRUE);
        GetDlgItem(IDC_EDIT_THRESHOLD)->EnableWindow(FALSE);
        GetDlgItem(IDC_BTN_RECOGNITION)->EnableWindow(TRUE);

        //将之前存储的信息清除
        m_curFaceInfo = { 0 };
        m_curVideoShowString = "";
        {
            lock_guard<mutex> lock(g_mutex);
            if (m_curVideoImage)
            {
                cvReleaseImage(&m_curVideoImage);
                m_curVideoImage = NULL;
            }
        }

        m_dataValid = false;
        m_videoOpened = false;

        Sleep(600);

        ClearShowWindow();

        if (m_hFDThread == NULL)
        {
            BOOL res = ::CloseHandle(m_hFDThread);
            if (!res)
            {
                GetLastError();
            }
        }

        m_bFDThreadRunning = FALSE;

        if (m_hFRThread == NULL)
        {
            ::CloseHandle(m_hFRThread);
        }

        GetDlgItem(IDC_BTN_CAMERA)->SetWindowText("启用摄像头");
    }
}

unsigned long _stdcall RunFaceDetectOperation(LPVOID lpParam)
{
    CArcFaceDemoDlg* dialog = (CArcFaceDemoDlg*)(lpParam);

    if (dialog == nullptr)
    {
        return 1;
    }

    cv::Mat frame;

    cv::VideoCapture capture;

    if (!capture.isOpened())
    {
        bool res = capture.open(0);
        if (res)
            dialog->m_videoOpened = true;
    }

    if (!(capture.set(CV_CAP_PROP_FRAME_WIDTH, VIDEO_FRAME_DEFAULT_WIDTH) &&
        capture.set(CV_CAP_PROP_FRAME_HEIGHT, VIDEO_FRAME_DEFAULT_HEIGHT)))
    {
        AfxMessageBox(_T("摄像头初始化失败！"));
        return 1;
    }

    while (dialog->m_videoOpened)
    {
        capture >> frame;

        ASF_SingleFaceInfo faceInfo = { 0 };

        IplImage image(frame);

        MRESULT detectRes = dialog->m_videoFaceEngine.PreDetectFace(&image, faceInfo);

        if (MOK == detectRes)
        {
            cvRectangle(&image, cvPoint(faceInfo.faceRect.left, faceInfo.faceRect.top),
                cvPoint(faceInfo.faceRect.right, faceInfo.faceRect.bottom), cvScalar(0, 0, 255), 2);

            dialog->m_curFaceInfo = faceInfo;
            dialog->m_dataValid = true;
        }
        else
        {
            //没有人脸不要显示信息
            dialog->m_curVideoShowString = "";
            dialog->m_dataValid = false;
        }


        //重新拷贝
        {
            lock_guard<mutex> lock(g_mutex);
            cvReleaseImage(&dialog->m_curVideoImage);
            dialog->m_curVideoImage = cvCloneImage(&image);
        }

        dialog->SendMessage(WM_PAINT);
    }

    capture.release();

    return 0;
}

unsigned long _stdcall RunFaceFeatureOperation(LPVOID lpParam)
{
    CArcFaceDemoDlg* dialog = (CArcFaceDemoDlg*)(lpParam);

    if (dialog == nullptr)
    {
        return 1;
    }

    //初始化特征
    ASF_FaceFeature faceFeature = { 0 };
    faceFeature.featureSize = FACE_FEATURE_SIZE;
    faceFeature.feature = (MByte *)malloc(faceFeature.featureSize * sizeof(MByte));

    while (dialog->m_bFDThreadRunning)
    {

        if (dialog->m_bLoadIconThreadRunning ||
            dialog->m_bClearFeatureThreadRunning)
        {
            //加载和清除注册库的过程中 不要显示信息
            dialog->m_curVideoShowString = "";
            continue;
        }

        if (!dialog->m_dataValid)
        {
            continue;
        }

        IplImage* tempImage = NULL;//先拷贝一份，防止读写冲突
        {
            lock_guard<mutex> lock(g_mutex);
            if (dialog->m_curVideoImage)
            {
                tempImage = cvCloneImage(dialog->m_curVideoImage);
            }
        }

        MRESULT detectRes = dialog->m_videoFaceEngine.PreExtractFeature(tempImage,
            faceFeature, dialog->m_curFaceInfo);

        cvReleaseImage(&tempImage);

        if (MOK != detectRes)
        {
            continue;
        }

        int maxIndex = 0;
        MFloat maxThreshold = 0.0;
        int curIndex = 0;

        if (dialog->m_bLoadIconThreadRunning ||
            dialog->m_bClearFeatureThreadRunning)
        {
            continue;
        }

        for each (auto regisFeature in dialog->m_featuresVec)
        {
            curIndex++;
            MFloat confidenceLevel = 0;
            MRESULT pairRes = dialog->m_videoFaceEngine.FacePairMatching(confidenceLevel, faceFeature, regisFeature);

            if (MOK == pairRes && confidenceLevel > maxThreshold)
            {
                maxThreshold = confidenceLevel;
                maxIndex = curIndex;
            }
        }

        if (atof(dialog->m_strEditThreshold) >= 0
            && maxThreshold >= atof(dialog->m_strEditThreshold))
        {
            CString resStr;
            resStr.Format("%d号 :%.2f", maxIndex, maxThreshold);
            //dialog->EditOut(resStr, TRUE);
            dialog->m_curVideoShowString = resStr;
        }
        else
        {
            dialog->m_curVideoShowString = "";
        }

        //dialog->SendMessage(WM_PAINT);
    }

    free(faceFeature.feature);
    return 0;
}


//双缓存画图
void CArcFaceDemoDlg::IplDrawToHDC(BOOL isVideoMode, IplImage* image, CRect& strShowRect, UINT ID)
{
    if (!image)
    {
        return;
    }

    CDC MemDC;

    CClientDC pDc(GetDlgItem(ID));

    //创建与窗口DC兼容的内存DC（MyDC）
    MemDC.CreateCompatibleDC(&pDc);

    IplImage* cutImg = cvCreateImage(cvSize(image->width - (image->width % 4), image->height), IPL_DEPTH_8U, image->nChannels);
    PicCutOut(image, cutImg, 0, 0);
    CBitmap* bmp = IplImage2CBitmap(cutImg);

    //把内存位图选进内存DC中用来保存在内存DC中绘制的图形
    CBitmap *oldbmp = MemDC.SelectObject(bmp);

    CPen pen(PS_SOLID, 4, RGB(255, 0, 0));
    pDc.SelectStockObject(NULL_BRUSH);

    pDc.SetBkMode(TRANSPARENT);
    pDc.SetTextColor(RGB(0, 0, 255));

    CRect rect;
    GetDlgItem(ID)->GetClientRect(&rect);

    //把内存DC中的图形粘贴到窗口中；
    pDc.SetStretchBltMode(HALFTONE);

    strShowRect.left = strShowRect.left < 0 ? 0 : strShowRect.left;
    strShowRect.top = strShowRect.top < 0 ? 0 : strShowRect.top;
    strShowRect.right = strShowRect.right > rect.right ? 0 : strShowRect.right;
    strShowRect.bottom = strShowRect.bottom > rect.bottom ? rect.bottom : strShowRect.bottom;

    if (isVideoMode)
    {
        pDc.StretchBlt(0, 0, rect.Width(), rect.Height(), &MemDC, 0, 0, VIDEO_FRAME_DEFAULT_WIDTH, VIDEO_FRAME_DEFAULT_HEIGHT, SRCCOPY);

        //为了让文字不贴边
        strShowRect.left += 4;
        strShowRect.top += 4;

        //画比对信息
        pDc.DrawText(m_curVideoShowString, &strShowRect, DT_TOP | DT_LEFT | DT_NOCLIP);
    }
    else
    {
        //图片由于尺寸不一致 ，需要重绘背景
        HBRUSH hBrush = ::CreateSolidBrush(RGB(255, 255, 255));
        ::FillRect(pDc.m_hDC, CRect(0, 0, m_windowViewRect.Width(), m_windowViewRect.Height()), hBrush);

        pDc.StretchBlt(m_curImageShowRect.X + 2, m_curImageShowRect.Y + 2,
            m_curImageShowRect.Width - 2, m_curImageShowRect.Height - 5, &MemDC, 0, 0, cutImg->width, cutImg->height, SRCCOPY);

        Gdiplus::Graphics graphics(pDc.m_hDC);

        graphics.DrawRectangle(&Pen(Gdiplus::Color::Red, 2), m_curFaceShowRect);

        //画age gender信息
        pDc.DrawText(m_curStaticShowAgeGenderString, &strShowRect, DT_TOP | DT_LEFT | DT_NOCLIP);

        //将比对信息放在age gender信息下
        strShowRect.top += 20;
        strShowRect.bottom += 20;

        //画比对信息
        pDc.DrawText(m_curStaticShowCmpString, &strShowRect, DT_TOP | DT_LEFT | DT_NOCLIP);
    }

    cvReleaseImage(&cutImg);

    //选进原来的位图，删除内存位图对象和内存DC
    MemDC.SelectObject(oldbmp);
    bmp->DeleteObject();
    MemDC.DeleteDC();

}


//图片格式转换
CBitmap* IplImage2CBitmap(const IplImage *img)
{
    if (!img)
    {
        return NULL;
    }

    CBitmap* bitmap = new CBitmap;//new一个CWnd对象
    BITMAPINFO bmpInfo;  //创建位图        
    bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmpInfo.bmiHeader.biWidth = img->width;
    bmpInfo.bmiHeader.biHeight = img->origin ? abs(img->height) : -abs(img->height);//img->height;//高度
    bmpInfo.bmiHeader.biPlanes = 1;
    bmpInfo.bmiHeader.biBitCount = 24;
    bmpInfo.bmiHeader.biCompression = BI_RGB;

    void *pArray = NULL;
    HBITMAP hbmp = CreateDIBSection(NULL, &bmpInfo, DIB_RGB_COLORS, &pArray, NULL, 0);//创建DIB，可直接写入、与设备无关，相当于创建一个位图窗口
    ASSERT(hbmp != NULL);
    UINT uiTotalBytes = img->width * img->height * 3;
    memcpy(pArray, img->imageData, uiTotalBytes);

    bitmap->Attach(hbmp);// 一个CWnd对象的HWND成员指向这个窗口句柄

    return bitmap;
}

void CArcFaceDemoDlg::OnEnChangeEditThreshold()
{
    //更新阈值
    UpdateData(TRUE);
    if (atof(m_strEditThreshold) < 0)
    {
        AfxMessageBox(_T("阈值必须大于0！"));
        SetDlgItemTextA(IDC_EDIT_THRESHOLD, "0.80");
    }

}


void CArcFaceDemoDlg::OnClose()
{
    // TODO: 在此添加消息处理程序代码和/或调用默认值
    if (m_videoOpened)
    {
        AfxMessageBox(_T("请先关闭摄像头！"));
        return;
    }

    CDialogEx::OnClose();

    m_bLoadIconThreadRunning = FALSE;
    TerminateLoadThread();
    m_bClearFeatureThreadRunning = FALSE;
    ClearRegisterImages();

    m_videoOpened = false;
    Sleep(500);

    m_bFDThreadRunning = FALSE;

    ::CloseHandle(m_hFDThread);
    ::CloseHandle(m_hFRThread);

    Sleep(500);

    m_imageFaceEngine.UnInitEngine();
    m_videoFaceEngine.UnInitEngine();
}

void CArcFaceDemoDlg::ClearShowWindow()
{
    //清空背景
    CDC* pCDC = GetDlgItem(IDC_STATIC_VIEW)->GetDC();
    HDC hDC = pCDC->m_hDC;
    HBRUSH hBrush = ::CreateSolidBrush(RGB(255, 255, 255));
    ::FillRect(hDC, CRect(0, 0, m_windowViewRect.Width(), m_windowViewRect.Height()), hBrush);
    DeleteObject(hBrush);
}


unsigned long _stdcall ClearFaceFeatureOperation(LPVOID lpParam)
{
    CArcFaceDemoDlg* dialog = (CArcFaceDemoDlg*)(lpParam);

    if (dialog == nullptr)
    {
        return 1;
    }

    int iImageCount = dialog->m_IconImageList.GetImageCount();

    dialog->m_IconImageList.Remove(-1);

    dialog->m_ImageListCtrl.DeleteAllItems();

    iImageCount = dialog->m_IconImageList.SetImageCount(0);

    //清除特征
    for (auto feature : dialog->m_featuresVec)
    {
        free(feature.feature);
    }

    dialog->m_featuresVec.clear();

    dialog->m_bClearFeatureThreadRunning = FALSE;

    return 0;
}