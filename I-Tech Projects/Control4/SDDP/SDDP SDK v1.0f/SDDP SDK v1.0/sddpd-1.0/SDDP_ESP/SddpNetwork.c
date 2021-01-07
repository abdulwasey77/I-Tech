/**************************************************************************//**

 @file     SddpNetwork.c

 @brief    Implements an SDDP network interface 
 
 @section  Detail Detail
            This module provides a generic network shim to the SDDP module. 
            The intent is to isolate the network functionality and allow 
            for easier porting to various paltforms. The module utilizes 
            the Berkeley socket API where possible.
            
 @section  Platforms Platform(s)
            LWIP, Linux, and other platforms making use Berkeley socket or 
            similar API

******************************************************************************/
///////////////////////////////////////
// Includes
///////////////////////////////////////
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#ifdef USE_LWIP // LWIP
#include "Lwiplib.h"
#include "DebugUtils.h"
#else // Linux and others
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <errno.h>
#include <fcntl.h>
#include <syslog.h>
#endif

#include "Sddp.h"
#include "SddpNetwork.h"


///////////////////////////////////////
// Defines
///////////////////////////////////////
#define INVALID_SOCKET  -1


///////////////////////////////////////
// Typedefs
///////////////////////////////////////


///////////////////////////////////////
// Variables
///////////////////////////////////////
#ifdef USE_LWIP
int errno;
#endif


///////////////////////////////////////
// Private Functions
///////////////////////////////////////

#ifndef USE_LWIP
static int IsSDDPInterface(const struct ifaddrs *ifa)
{
	if (ifa->ifa_addr->sa_family != AF_INET)
		return 0;
	if ((ifa->ifa_flags & (IFF_UP | IFF_MULTICAST | IFF_LOOPBACK | IFF_POINTOPOINT)) != (IFF_UP | IFF_MULTICAST))
		return 0;
	return 1;
}
#endif

static SDDPInterface *SDDPGetLocalInterface(void)
{
	SDDPInterface *ret;
	char *ip;

	ret = malloc(sizeof(ret[0]));
	if (ret)
	{
		memset(ret, 0, sizeof(ret[0]));
		ret[0].addr.s_addr = htonl(INADDR_LOOPBACK);
		ip = inet_ntoa(ret[0].addr);
		if (ip)
			strcpy(ret[0].ip_addr, ip);
	}
	
	return ret;
}

///////////////////////////////////////
// Public Functions
///////////////////////////////////////

/**************************************************************************//**
 @fn        int SDDPNetworkIsLocalAddress(struct in_addr addr)

 @brief     Returns whether the IP address is considered local.

 @param     addr - The IP address.
 
 @return    1 if the IP address is considered local
            0 if the IP address is not considered local

*******************************************************************************/
int SDDPNetworkIsLocalAddress(struct in_addr addr)
{
    SDDPInterface *ifaces, *ifc;
    int ret = 0;
    
    if (addr.s_addr == htonl(INADDR_LOOPBACK))
        return 1;
    
    ifaces = SDDPGetInterfaces(0);
    if (ifaces)
    {
        for (ifc = ifaces; ifc; ifc = ifc->next)
        {
            if (addr.s_addr == ifc->addr.s_addr)
            {
                ret = 1;
                break;
            }
        }
        
        free(ifaces);
    }
    
    return ret;
}

/**************************************************************************//**
 @fn        SDDPInterface *SDDPGetInterfaces(void)


 @brief     Returns an array of SDDPInterface structures that represent all 
            network interfaces used for SDDP.
            

            The returned array of SDDPInterface structures must be freed using
            free().


 @return    Array of SDDPInterface structures if ok
            NULL if error


*******************************************************************************/
SDDPInterface *SDDPGetInterfaces(int local_only)
{
#ifdef USE_LWIP
	SDDPInterface *ret;
	char *ip;
	
	if (local_only)
	    return SDDPGetLocalInterface();
	
	ret = malloc(sizeof(*ret));
	if (ret)
	{
		memset(ret, 0, sizeof(*ret));
		ret->addr.addr = lwIPLocalIPAddrGet();
		ip = inet_ntoa(ret->addr);
		if (ip)
			strcpy(ret->ip_addr, ip);
	}
	
	return ret;
#else
	char *ip;
	struct ifaddrs *ifaddr, *ifa;
	SDDPInterface *ret = NULL;
	int i, cnt = 0;
	
	if (local_only)
	    return SDDPGetLocalInterface();
	
	if (getifaddrs(&ifaddr) < 0)
		return NULL;
	
	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
	{
		if (IsSDDPInterface(ifa))
			cnt++;
	}
	
	if (cnt > 0)
	{
		ret = malloc(cnt * sizeof(ret[0]));
		if (ret)
		{
			memset(ret, 0, cnt * sizeof(ret[0]));
			
			i = 0;
			for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
			{
				if (!IsSDDPInterface(ifa))
					continue;
				
				if (i > 0)
					ret[i - 1].next = &ret[i];
				
				ret[i].addr = ((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
				ip = inet_ntoa(ret[i].addr);
				if (ip)
					strcpy(ret[i].ip_addr, ip);
				i++;
			}
		}
	}
	else
	{
		// No network interface: Create a default one
		ret = malloc(sizeof(ret[0]));
		if (ret)
		{
			memset(ret, 0, sizeof(ret[0]));
			ret[0].addr.s_addr = htonl(INADDR_LOOPBACK);
			ip = inet_ntoa(ret[0].addr);
			if (ip)
				strcpy(ret[0].ip_addr, ip);
		}
	}
	
	freeifaddrs(ifaddr);
	return ret;
#endif
}

/**************************************************************************//**
 @fn        SDDPStatus SDDPNetworkInit(SDDPNetworkInfo *network)

 @brief     Opens the sockets used by SDDP to receive and send multicast identify
            and announce packets, notifies, etc. 'notify_socket' is used for
            sending both multicast and unicast. 'listening_socket' is used for 
            reciving SDDP traffic on SDDP_PORT. 
            
            This function also joins the mulicast group via IGMP for the SDDP multicast
            IP address. 

 @param     network - pointer to network file descriptors

 @return    SDDP_STATUS_SUCCESS if ok
            SDDP_STATUS_NETWORK_ERROR on networking initialization error

*******************************************************************************/
SDDPStatus SDDPNetworkInit(SDDPNetworkInfo *network)
{
    int socket_opt = 0;
#if 0
    unsigned char loop;
#endif
    struct sockaddr_in local;
    int flags;

    syslog(LOG_DEBUG, "SDDP Network init");

    // Init global sddp data
    network->notify_socket = INVALID_SOCKET;
    network->listener_socket = INVALID_SOCKET;

    // Setup generic send socket used for unicast and multicast notify messages	
    network->notify_socket = socket(AF_INET, SOCK_DGRAM,  0); // IPPROTO_UDP implicit from SOCK_DGRAM
    if (network->notify_socket < 0)
    {
        syslog(LOG_INFO, "Failed to create SDDP unicast socket %d", network->notify_socket);
        return SDDP_STATUS_NETWORK_ERROR;
    }

#if 0
    // Disable looping packets back to the local host (this is not always reliable)
    loop = 0;
    if (setsockopt(network->notify_socket, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop)))
    {
        syslog(LOG_INFO, "Can't set IP_MULTICAST_LOOP on notify_socket socket: %s", strerror(errno));
    }
#endif
    
    // Setup TTL
    socket_opt = SDDP_TTL;
    if (setsockopt(network->notify_socket, IPPROTO_IP, IP_MULTICAST_TTL, &socket_opt, sizeof(int)))
    {
        syslog(LOG_INFO, "Can't set IP_MULTICAST_TTL on multicast socket: %s", strerror(errno));
    }

    // Create a listener socket on SDDP port for search requests
    syslog(LOG_DEBUG, "Creating multicast socket");
    network->listener_socket = socket(PF_INET, SOCK_DGRAM, 0); // IPPROTO_UDP is implicit from SOCK_DGRAM
    if (network->listener_socket < 0)
    {
        syslog(LOG_ALERT, "Failed to create SDDP multicast listener socket %d", network->listener_socket);
        return SDDP_STATUS_NETWORK_ERROR;
    }

    // Reuse address (allow for quick restart)
    socket_opt = 1;
    if (setsockopt(network->listener_socket, SOL_SOCKET, SO_REUSEADDR, &socket_opt, sizeof(int)))
    {
        syslog(LOG_INFO, "Can't set SO_REUSEADDR on multicast socket: %s", strerror(errno));
    }

#ifdef USE_LWIP
    flags = 1;
    ioctlsocket(network->listener_socket, FIONBIO, &flags);
#else
    // Setup as non-blocking, single tick call processes multiple sockets
    flags = fcntl(network->listener_socket, F_GETFL,0);
    fcntl(network->listener_socket, F_SETFL, flags | O_NONBLOCK);
#endif    

    // Bind socket to address
    local.sin_family = AF_INET;
    local.sin_port = htons(SDDP_PORT);
    local.sin_addr.s_addr = inet_addr(SDDP_MULTI_ADDR);     
    if (bind(network->listener_socket, (struct sockaddr const *)&local, sizeof(struct sockaddr_in)) < 0)
    {
        syslog(LOG_ALERT, "Failed to bind SDDP multicast socket: %s", strerror(errno));
        close(network->listener_socket);
        network->listener_socket = INVALID_SOCKET;
        return SDDP_STATUS_NETWORK_ERROR;
    }


    // Join to multicast group. This is one of the more platform specific sections of code. 
    // Various implementations will do this different ways, so it is left to the implementor
    // to define the best strategy.
    syslog(LOG_DEBUG, "Joining multicast group for %s", SDDP_MULTI_ADDR);    
#ifdef USE_LWIP
    // LWIP IGMP method
    {
        struct ip_addr multicast_ip;
        struct ip_addr local_ip;
        int status;

        multicast_ip.addr = inet_addr(SDDP_MULTI_ADDR);
        local_ip.addr = lwIPLocalIPAddrGet(); 
        status = igmp_joingroup(&local_ip, &multicast_ip);
        if (status != ERR_OK)
        {
            syslog(LOG_ALERT, "SDDP IGMP join %s group failed %d", SDDP_MULTI_ADDR, status);
            return SDDP_STATUS_NETWORK_ERROR;
        }
    }
#else
    // More generic sockopt method
    {
        struct ip_mreq mreq;
        syslog(LOG_DEBUG, "Adding group membership");
        mreq.imr_multiaddr.s_addr = inet_addr(SDDP_MULTI_ADDR);
        mreq.imr_interface.s_addr = htonl(INADDR_ANY); // Let kernal determine interface
        if (setsockopt(network->listener_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void *)&mreq, sizeof(struct ip_mreq)))
        {
            syslog(LOG_ALERT, "Failed to join multicast group: %s", strerror(errno));
            return SDDP_STATUS_NETWORK_ERROR;
        }
    }   
#endif
    return SDDP_STATUS_SUCCESS;
}

/***************************************************************************//**
 @fn        void SDDPNetworkClose(SDDPNetworkInfo *network)

 @brief     Closes sockets and cleans up network interface. 

 @param     network - pointer to network file descriptors

 @return    void
*******************************************************************************/
void SDDPNetworkClose(SDDPNetworkInfo *network)
{
    syslog(LOG_DEBUG, "Network close called");

    // delete listener socket
    if (network->listener_socket != INVALID_SOCKET)
    {
        syslog(LOG_INFO, "Closing multicast socket %d", network->listener_socket);
        close(network->listener_socket);
        network->listener_socket = INVALID_SOCKET;
    }

    // delete notify socket
    if (network->notify_socket != INVALID_SOCKET)
    {
        syslog(LOG_INFO, "Closing unicast socket %d", network->notify_socket);
        close(network->notify_socket);
        network->notify_socket = INVALID_SOCKET;
    }
}


/***************************************************************************//**
 @fn        SDDPStatus SDDPNetworkAvailable(SDDPNetworkInfo *network)

 @brief     Checks if the SDDP network interface is available

 @param     network - pointer to network file descriptors

 @return    SDDP_STATUS_SUCCESS if network interface is ready
            SDDP_STATUS_NETWORK_ERROR on network error (could not create sockets)

*******************************************************************************/
SDDPStatus SDDPNetworkAvailable(SDDPNetworkInfo *network)
{
    if (network->notify_socket == INVALID_SOCKET || network->listener_socket == INVALID_SOCKET)
    {
        // Can't generate SDDP messages without viable sockets
        return SDDP_STATUS_NETWORK_ERROR;
    }
    else
    {
        return SDDP_STATUS_SUCCESS;
    }
}

/***************************************************************************//**
 @fn        int SDDPNetworkReceive(SDDPNetworkInfo *network, char *buf, int buf_len, 
                       struct sockaddr *from, unsigned int *from_len)

 @brief     Receives incoming UDP message and returns length received

 @param     network - pointer to network file descriptors
 @param     buf - pointer to data buffer to store incoming data
 @param     buf_len - length of data buffer
 @param     from - socket info on message sender
 @param     from_len - length of socket info

 @return    length received
            <0 otherwise (actual value is platform dependent)

*******************************************************************************/
int SDDPNetworkReceive(SDDPNetworkInfo *network, char *buf, int buf_len, 
                       struct sockaddr *from, unsigned int *from_len)
{
    return recvfrom(network->listener_socket, buf, buf_len, 0, from, from_len);
}


/***************************************************************************//**
 @fn        SDDPStatus SDDPNetworkSendMulticast(SDDPNetworkInfo *network, 
                                                         const char *msg)

 @brief     Sends SDDP payload multicast or unicast. Uses network buffers.

 @param     msg - pointer to SDDP formatted payload

 @return    SDDP_STATUS_SUCCESS if ok
            SDDP_STATUS_INVALID_PARAM if unable to allocate buffer
            SDDP_STATUS_NETWORK_ERROR on network error

*******************************************************************************/
SDDPStatus SDDPNetworkSendMulticast(SDDPNetworkInfo *network, const SDDPInterface *iface, const char *msg)
{
    struct sockaddr_in send_to;
    struct in_addr iface_addr;
    
    int len = strlen(msg);
    
    iface_addr = iface->addr;
    if (setsockopt(network->notify_socket, IPPROTO_IP, IP_MULTICAST_IF, &iface_addr, sizeof(iface_addr)) < 0)
    {
        syslog(LOG_INFO, "Multicast setting interface %s failed %s", inet_ntoa(iface_addr), strerror(errno));
        return SDDP_STATUS_NETWORK_ERROR;
    }

    send_to.sin_family = AF_INET;
    send_to.sin_port = htons(SDDP_PORT); 
    send_to.sin_addr.s_addr = inet_addr(SDDP_MULTI_ADDR);
    if (sendto(network->notify_socket, msg, len, 0, (struct sockaddr *)&send_to, sizeof (struct sockaddr_in)) > 0)
    {
        return SDDP_STATUS_SUCCESS;
    }
    else
    {
        syslog(LOG_INFO, "Multicast send failed %s", strerror(errno));
        return SDDP_STATUS_NETWORK_ERROR;
    }
}


/***************************************************************************//**
 @fn        SDDPStatus SDDPNetworkSendTo(SDDPNetworkInfo *network, const char *msg, 
                                                        struct sockaddr_in *send_to)

 @brief     Sends SDDP payload multicast or unicast

 @param     network - pointer to network file descriptors
 @param     msg - pointer to SDDP formatted payload
 @param     send_to - remote host to send to. Can be a unicast IP and port or multicast

 @return    SDDP_STATUS_SUCCESS if ok
            SDDP_STATUS_INVALID_PARAM if unable to allocate buffer
            SDDP_STATUS_NETWORK_ERROR on network error

*******************************************************************************/
SDDPStatus SDDPNetworkSendTo(SDDPNetworkInfo *network, const char *msg, struct sockaddr_in *send_to)
{
    int len = strlen(msg);

    if (send_to->sin_addr.s_addr == 0)
    {
        syslog(LOG_ALERT, "Failed - unicast address not provided");
        return SDDP_STATUS_INVALID_PARAM;
    }

    if (sendto(network->notify_socket, msg, len, 0,(struct sockaddr *)send_to, sizeof (struct sockaddr_in)) > 0)
    {
        return SDDP_STATUS_SUCCESS;
    }
    else
    {
        syslog(LOG_ALERT, "Unicast send failed %s", strerror(errno));
        return SDDP_STATUS_NETWORK_ERROR;
    }
}


