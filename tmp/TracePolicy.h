#pragma once

#include "../libzqpkt_img_fifo/simple_student_trace/KVConfig.h"

/** �����л�����
 */
class TracePolicy
{
	KVConfig *cfg_, *cfg_policy_;

public:
	TracePolicy(KVConfig *cfg);
	~TracePolicy(void);

	/// ������� N ��û��Ŀ�꣬���������ص���ʼλ��
	double time_to_reset_ptz() const;

	/// ����Ŀ����Ϊ��ʱ�䳤�ȣ����� N ���ڣ�������
	double analyze_interval() const;

	/// ������� N ��û��Ŀ�꣬����ֹ̨ͣ
	double time_to_stop_tracing() const;

	/// ��ʧĿ��������೤ʱ�䣬Ӧ���л���ȫ��
	double time_to_fullview_after_no_target() const;

	/// �л�����������Ҫ�ȶ���ʱ��
	double time_to_closeview() const;

private:
	void load_policy_data();
};
