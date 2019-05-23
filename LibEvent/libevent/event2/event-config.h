#ifndef EVENT2_EVENT_CONFIG_H__
#define EVENT2_EVENT_CONFIG_H__

#ifdef _WIN32
#include <event2/event-config-win32.h>
#else
#include <event2/event-config-linux.h>
#endif

#endif