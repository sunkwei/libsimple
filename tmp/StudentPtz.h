#pragma once

#include "../libzqpkt_img_fifo/simple_student_trace/KVConfig.h"
#include "Ptz.h"

/** 控制学生云台 */
class StudentPtz : public Ptz
{
	KVConfig *cfg_;

	int state_;	// 0 enable, 1 disable

public:
	StudentPtz(KVConfig *cfg);
	~StudentPtz(void);
	void set_running_state(int state) { state_ = state; }

};
