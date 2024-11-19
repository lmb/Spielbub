#pragma once

#define offsetofend(type, member) \
    (offsetof(type, member) + sizeof(((type*)0)->member))

#define range_of(type, member) \
	offsetof(type, member) ... (offsetofend(type, member) - 1)
