#ifndef __TERMINAL_NETWORKING_H__
#define __TERMINAL_NETWORKING_H__

#include <shinobi.h>

typedef enum
{
	NET_STATUS_NODEVICE,
	NET_STATUS_CONNECTED,
	NET_STATUS_DISCONNECTED,
	NET_STATUS_ERROR
}NET_STATUS;

typedef enum
{
	NET_DEVICE_TYPE_LAN,
	NET_DEVICE_TYPE_BBA,
	NET_DEVICE_TYPE_EXTMODEM,
	NET_DEVICE_TYPE_SERIALPPP,
	NET_DEVICE_TYPE_INTMODEM,
	NET_DEVICE_TYPE_NONE
}NET_DEVICE_TYPE;

int NET_Initialise( void );
void NET_Terminate( void );

NET_STATUS NET_GetStatus( void );
NET_DEVICE_TYPE NET_GetDeviceType( void );

#endif /* __TERMINAL_NETWORKING_H__ */

