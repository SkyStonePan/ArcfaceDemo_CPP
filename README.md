快速上手：

    环境配置：
        1.	安装VS2013环境安装包(vcredist_x86_vs2013.exe)
        2.	从官网(http://www.arcsoft.com.cn/ai/arcface.html)申请sdk，下载对应的sdk版本(x86或x64)并解压
        3.	头文件配置：头文件放入\ArcFaceDemo\include\inc文件夹内
        4.	SDK库配置：
            x64版本：
                将libarcsoft_face_engine.lib放至\ArcFaceDemo\lib64\FreeSdk文件夹下,如果没有对应的文件夹，请先创建
            x86版本：
                将libarcsoft_face_engine.lib放至\ArcFaceDemo\lib32\FreeSdk文件夹下,如果没有对应的文件夹，请先创建
        5.	在运行代码的时候将对应版本SDK和OpenCV的dll库放至项目根目录下，以免运行时找不到对应的dll库
        6.	将官网申请的APPID及SDKKEY填写至ArcFaceEngine.cpp文件中，注意平台和版本对应
        7.	在Debug或者Release中选择配置管理器，选择对应的平台
        8.	Demo使用多字节字符集，如果报MSB8031错误，请下载安装vc_mbcsmfc.exe(https://www.microsoft.com/en-us/download/details.aspx?id=40770)
    运行程序：
        1.	按F5启动程序
        2.	点击“注册人脸”按钮增加人脸库图片，针对每张照片只提取最大人脸进行检测以及人脸特征检测并保存到缓存中
        3.	点击“选择识别照”按钮增加识别照图片，左上角显示检测到的年龄、性别信息
        4.	点击“人脸比对”按钮进行人脸比对，左上角显示比对结果
        5.	点击“启用摄像头”按钮可以打开摄像头并自动与人脸库进行比对，并动态显示结果
        6.	点击“清除人脸库”按钮，即清除当前人脸库
	7.	程序执行信息会在界面底部中的编辑框中进行显示
	8.	阈值框内在启用摄像头的情况下可以自由更改，只有达到阈值的比对信息才会显示


常见问题：

	1.启动后引擎初始化失败	
		(1)请选择对应的平台，如x64,x86 
		(2)删除工程目录下对应的asf_install.dat，freesdk_132512.dat
		(3)请确保ArcFaceEngine.cpp中appid和appkey与当前sdk版本一致 

	2.SDK支持那些格式的图片人脸检测？	
		目前SDK支持的图片格式有jpg，jpeg，png，bmp等。
		
	3.使用人脸检测功能对图片大小有要求吗？	
		推荐的图片大小最大不要超过2M，因为图片过大会使人脸检测的效率不理想，当然图片也不宜过小，否则会导致无法检测到人脸。
		
	4.使用人脸识别引擎提取到的人脸特征信息是什么？	
		人脸特征信息是从图片中的人脸上提取的人脸特征点，是unsigned char* 格式。 
		
	5.SDK人脸比对的阈值设为多少合适？	
		推荐值为0.8，用户可根据不同场景适当调整阈值。
		
	6.可不可以将人脸特征信息保存起来，等需要进行人脸比对的时候直接拿保存好的人脸特征进行比对？
		可以，当人脸个数比较多时推荐先存储起来，在使用时直接进行比对，这样可以大大提高比对效率。存入数据库时，请以Blob的格式进行存储，不能以string或其他格式存储。
		
	7.图片中有人脸，但是检测时未检测到人脸	
		(1)请调整detectFaceScaleVal的值
		(2)请确认图片的宽度是否为4的倍数
    
    8.编译错误MSB8031
        请下载安装vc_mbcsmfc.exe (https://www.microsoft.com/en-us/download/details.aspx?id=40770)。
    
    更多常见问题请访问 https://ai.arcsoft.com.cn/manual/faqs.html。
