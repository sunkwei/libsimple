#include "TracePolicy.h"

TracePolicy::TracePolicy(KVConfig *cfg)
	: cfg_(cfg)
{
	cfg_policy_ = 0;

	load_policy_data();
}

TracePolicy::~TracePolicy(void)
{
	delete cfg_policy_;
}

void TracePolicy::load_policy_data()
{
	const char *name = cfg_->get_value("trace_policy", "simple.trace.policy.config");
	delete cfg_policy_;
	cfg_policy_ = new KVConfig(name);
}

double TracePolicy::time_to_reset_ptz() const
{
	return atof(cfg_policy_->get_value("time_to_reset_ptz", "5.0"));
}

double TracePolicy::time_to_stop_tracing() const
{
	return atof(cfg_policy_->get_value("time_to_stop_tracing", "0.8"));
}

double TracePolicy::analyze_interval() const
{
	return atof(cfg_policy_->get_value("analyze_interval", "3.0"));
}

double TracePolicy::time_to_fullview_after_no_target() const
{
	return atof(cfg_policy_->get_value("time_to_fullview_after_no_target", "1.0"));
}

double TracePolicy::time_to_closeview() const
{
	return atof(cfg_policy_->get_value("time_to_closeview", "1.5"));
}
