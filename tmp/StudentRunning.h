/** ѧ����̽�⴦�� ...
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
	Coordinate_Conversion cc_;	// ���ڴ�̽�⵽���ٵ�����ת��
	Camera_parameter cp_detecting_, cp_tracing_; // �������

	enum RunningState
	{
		RUNNING,		// ����
		PTZINITING,		// ��̨��ʼλ��
		CALIBRATING,	// ѧ�����궨
		CALIBRATING2,
		CALIBRATING3,
	};

	RunningState running_state_;

	struct CalibrationPoint
	{
		cv::Point pt;	// �궨���λ��
	};
	typedef std::vector<CalibrationPoint> CALIBRATIONS;
	CALIBRATIONS calibrations_;		// �궨����У��ɴ�����Ϊ��տ�
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
