/** 学生的探测处理 ...
 */

#pragma once

#include <opencv2/opencv.hpp>
#include "tdt.h"
#include "../libzqpkt_img_fifo/simple_student_trace/KVConfig.h"
#include "Running.h"
#include "../../libptz/ptz.h"
#include "../teacher_detect_trace/ZoomValueConvert.h"
#include "../simple_teacher_trace/fullimage_recognition.h"
#include "TracePolicy.h"
#include <algorithm>
#include "StudentPtz.h"
#include "StudentDetecting.h"

class StudentRunning : public Running
{
	StudentPtz *ptz_;
	StudentDetecting *det_;
	Coordinate_Conversion cc_;	// 用于从探测到跟踪的坐标转换
	Camera_parameter cp_detecting_, cp_tracing_; // 相机参数

	enum RunningState
	{
		RUNNING,		// 运行
		PTZINITING,		// 云台初始位置
		CALIBRATING,	// 学生区标定
		CALIBRATING2,
		CALIBRATING3,
	};

	RunningState running_state_;

	struct CalibrationPoint
	{
		cv::Point pt;	// 标定点的位置
	};
	typedef std::vector<CalibrationPoint> CALIBRATIONS;
	CALIBRATIONS calibrations_;		// 标定点队列，由此连接为封闭框
	ost::Mutex cs_calibrations_;

public:
	StudentRunning(WindowThread *th, KVConfig *cfg);
	~StudentRunning(void);

private:
	virtual void one_image(IplImage *raw_img, IplImage *masked_img, double stamp);
	virtual bool is_teacher() const { return false; }
	virtual int key_handle(int key);
	virtual void mouse_detected(int ev, int x, int y, int flags);
	virtual void mouse_traced(int ev, int x, int y, int flags);

	void load_camera_params();

	void key_handle_running(int key);
	void key_handle_ptziniting(int key);
	void key_handle_calibrating(int key);

	void show_info(IplImage *img);
	void show_calibrating(IplImage *img);
};
