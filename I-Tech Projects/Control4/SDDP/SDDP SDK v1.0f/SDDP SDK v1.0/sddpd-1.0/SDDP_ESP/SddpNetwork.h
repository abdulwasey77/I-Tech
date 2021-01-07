/**************************************************************************//**

 @file     SddpNetwork.h
 @brief    ...
 @section  Platforms Platform(s)
            All

******************************************************************************/
#ifndef _SDDP_NETWORK_H_
#define _SDDP_NETWORK_H_

#ifdef __cplusplus
extern "C" {
#endif

///////////////////////////////////////
// Includes
///////////////////////////////////////
#ifdef USE_LWIP
#include "Lwiplib.h"
#else
#include <netinet/in.h>
#endif

#include "SddpStatus.h" // Include SDDP status codes

///////////////////////////////////////
// Defines
///////////////////////////////////////
// Network Settings
#define SDDP_TTL                32 
#define SDDP_MULTI_ADDR         "239.255.255.250"
#define SDDP_PORT               1902

#ifdef USE_LWIP
#define SDDP_NETIF              "lm0"
#endif

///////////////////////////////////////
// Typedefs
///////////////////////////////////////
/**************************************************************************//**
 @typedef  struct SDDPNetworkInfo
           Provides socket file descriptors for the active network connections
           used by SDDP. 
******************************************************************************/
typedef struct
{
    int    notify_socket;   // Socket used to send unicast and multicast
    int    listener_socket; // Listens for incoming SDDP traffic
} SDDPNetworkInfo;

/**************************************************************************//**

 @typedef  struct SDDPInterface
           Provides information for available network interfaces that can be 
           used by SDDP. 
******************************************************************************/
typedef struct _SDDPInterface
{
	struct _SDDPInterface *next; // Pointer to next structure, or NULL if this is the last in the array
#ifdef USE_LWIP
	struct ip_addr addr;
#else
	struct in_addr addr;
#endif
	char ip_addr[32];            // Interface IP address as as string
} SDDPInterface;


///////////////////////////////////////
// Functions
///////////////////////////////////////
/**************************************************************************//**
 @fn        int SDDPNetworkIsLocalAddress(struct in_addr addr)

 @brief     Returns whether the IP address is considered local.

 @param     addr - The IP address.
 
 @return    1 if the IP address is considered local
            0 if the IP address is not considered local

*******************************************************************************/
int SDDPNetworkIsLocalAddress(struct in_addr addr);

/**************************************************************************//**
 @fn        SDDPInterface *SDDPGetInterfaces(void)

 @brief     Returns an array of SDDPInterface structures that represent all 
            network interfaces used for SDDP.
            
            The returned array of SDDPInterface structures must be freed using
            free().

 @return    Array of SDDPInterface structures if ok
            NULL if error

*******************************************************************************/
SDDPInterface *SDDPGetInterfaces(int local_only);

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
SDDPStatus SDDPNetworkInit(SDDPNetworkInfo *network);


/***************************************************************************//**
 @fn        void SDDPNetworkClose(SDDPNetworkInfo *network)

 @brief     Closes sockets and cleans up network interface. 

 @param     network - pointer to network file descriptors

 @return    void
*******************************************************************************/
void SDDPNetworkClose(SDDPNetworkInfo *network);


/***************************************************************************//**
 @fn        SDDPStatus SDDPNetworkAvailable(SDDPNetworkInfo *network)

 @brief     Checks if the SDDP network interface is available

 @param     network - pointer to network file descriptors

 @return    SDDP_STATUS_SUCCESS if network interface is ready
            SDDP_STATUS_NETWORK_ERROR on network error (could not create sockets)

*******************************************************************************/
SDDPStatus SDDPNetworkAvailable(SDDPNetworkInfo *network);


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
int SDDPNetworkReceive(SDDPNetworkInfo *network, char *buf, int buf_len, struct sockaddr *from, unsigned int *from_len);


/***************************************************************************//**
 @fn        SDDPStatus SDDPNetworkSendMulticast(SDDPNetworkInfo *network, 
                                                         const char *msg)

 @brief     Sends SDDP payload multicast or unicast. Uses network buffers.

 @param     msg - pointer to SDDP formatted payload

 @return    SDDP_STATUS_SUCCESS if ok
            SDDP_STATUS_INVALID_PARAM if unable to allocate buffer
            SDDP_STATUS_NETWORK_ERROR on network error

*******************************************************************************/
SDDPStatus SDDPNetworkSendMulticast(SDDPNetworkInfo *network, const SDDPInterface *iface, const char *msg);


/***************************************************************************//**
 @fn        SDDPStatus SDDPNetworkSendTo(SDDPNetworkInfo *network, const char *msg, 
                                                        struct sockaddr_in *send_to)

 @brief     Sends SDDP payload multicast or unicast

 @param     network - pointer to network file descriptors
 @param     msg - pointer to SDDP formatted payload
 @param     send_to - remote host to send to

 @return    SDDP_STATUS_SUCCESS if ok
            SDDP_STATUS_INVALID_PARAM if unable to allocate buffer
            SDDP_STATUS_NETWORK_ERROR on network error

*******************************************************************************/
SDDPStatus SDDPNetworkSendTo(SDDPNetworkInfo *network, const char *msg, struct sockaddr_in *send_to);


#ifdef __cplusplus
}
#endif
#endif
