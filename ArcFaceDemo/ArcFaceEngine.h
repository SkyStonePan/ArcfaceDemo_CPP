#pragma once

#include "stdafx.h"
#include "merror.h"
#include "arcsoft_face_sdk.h"
#include <opencv2\opencv.hpp>
#include <vector>

class ArcFaceEngine
{
public:
	ArcFaceEngine();
	~ArcFaceEngine();

	MRESULT ActiveSDK();				//激活SDK
	MRESULT InitEngine(MLong mode);		//初始化
	MRESULT UnInitEngine();				//反初始化
	MRESULT PreExtractFeature(IplImage* image, ASF_FaceFeature& feature, ASF_SingleFaceInfo& faceRect); //FR
	MRESULT PreDetectFace(IplImage* image, ASF_SingleFaceInfo& faceRect);                               //FD
	MRESULT FacePairMatching(MFloat &confidenceLevel, ASF_FaceFeature feature1, ASF_FaceFeature feature2);			//人脸对比
	MRESULT FaceASFProcess(ASF_MultiFaceInfo detectedFaces, IplImage *img, ASF_AgeInfo &ageInfo,
		ASF_GenderInfo &genderInfo, ASF_Face3DAngle &angleInfo);									//Process
	const ASF_VERSION* GetVersion();


private:
	MHandle m_hEngine;
};

void PicCutOut(IplImage* src, IplImage* dst, int x, int y);