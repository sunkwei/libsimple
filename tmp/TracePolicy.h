#pragma once

#include "../libzqpkt_img_fifo/simple_student_trace/KVConfig.h"

/** 跟踪切换策略
 */
class TracePolicy
{
	KVConfig *cfg_, *cfg_policy_;

public:
	TracePolicy(KVConfig *cfg);
	~TracePolicy(void);

	/// 如果连续 N 秒没有目标，则跟踪相机回到初始位置
	double time_to_reset_ptz() const;

	/// 分析目标行为的时间长度，如在 N 秒内，如何如何
	double analyze_interval() const;

	/// 如果连续 N 秒没有目标，则云台停止
	double time_to_stop_tracing() const;

	/// 丢失目标后，连续多长时间，应该切换回全景
	double time_to_fullview_after_no_target() const;

	/// 切换到近景，需要稳定的时间
	double time_to_closeview() const;

private:
	void load_policy_data();
};
