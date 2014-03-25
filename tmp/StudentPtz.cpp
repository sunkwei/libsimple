#include "StudentPtz.h"

StudentPtz::StudentPtz(KVConfig *cfg)
	: cfg_(cfg)
	, Ptz(cfg)
{
}

StudentPtz::~StudentPtz(void)
{
}
