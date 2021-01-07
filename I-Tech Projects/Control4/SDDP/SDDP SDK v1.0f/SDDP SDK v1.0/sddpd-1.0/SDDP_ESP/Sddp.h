/**************************************************************************//**

 @file     SDDP.h
 @brief    ...
 @section  Platforms Platform(s)
            All
            
******************************************************************************/
#ifndef _SDDP_H_
#define _SDDP_H_

#ifdef __cplusplus
extern "C" {
#endif

///////////////////////////////////////
// Includes
///////////////////////////////////////
#include <stdbool.h>
#include <time.h>

#ifdef USE_LWIP // LWIP 
#include <lwip/sockets.h>
#else // Linux and other platforms
#include <net/if.h>
#endif

#include "SddpStatus.h"
#include "SddpNetwork.h"


///////////////////////////////////////
// Defines
///////////////////////////////////////
#define MIN_ADVERT_PERIOD         60  // Minimum allow advertisement period (0 is disabled)
#define DISABLE_PERIODIC_ADVERTS  0   // Provide as 'max_age' to disable period advertisements
#define MAX_SDDP_FRAME_SIZE       512 // local buffer size allowed for incoming and outgoing SDDP
#define MAX_HOST_SIZE             100


///////////////////////////////////////
// Typedefs
///////////////////////////////////////
/**************************************************************************//**
 @typedef  SDDPHandle
           A Handle used to interact with the SDDP library
******************************************************************************/
typedef void * SDDPHandle;

///////////////////////////////////////
// Functions
///////////////////////////////////////
/***************************************************************************//**
 @fn        SDDPStatus SDDPInit(SDDPHandle *handle)

 @brief     Initialize the SDDP handle.

 @param     handle - ptr to new handle to SDDP, caller must free with SDDPShutdown

 @return    SDDP_STATUS_SUCCESS on success
*******************************************************************************/
SDDPStatus SDDPInit(SDDPHandle *handle);

/***************************************************************************//**
 @fn        SDDPStatus SDDPSetHost(const SDDPHandle handle, const char * host)

 @brief     Sets the hostname for SDDP to advertise

 @param     handle - handle to SDDP
 @param     host - hostname to use, will be copied (up to MAX_HOST_SIZE)

 @return    SDDP_STATUS_SUCCESS on success
            SDDP_STATUS_INVALID_PARAM on invalid parameters
*******************************************************************************/
SDDPStatus SDDPSetHost(const SDDPHandle handle, const char * host);

/***************************************************************************//**
 @fn        SDDPStatus SDDPSetDevice(const SDDPHandle handle, int index, const char *product_name,
                                     const char *primary_proxy, const char *proxies,
                                     const char *manufacturer, const char *model,
                                     const char *driver, int max_age, int local_only)


 @brief     Sets up the device info for SDDP to advertise.

 @param     handle - handle to SDDP
            index - index of device (usually 0, but there can be multiple devices)
            product_name - Product name for SDDP search target - i.e. "c4:television"
            primary_proxy - Control4 primary proxy type - i.e. "TV"
            proxies - All Control4 proxy types support - i.e. "TV,DVD"
            manufacturer - Manufacturer - i.e. "Control4"
            model - Model number - i.e. "C4-105HCTV2-EB"
            driver - Control4 driver c4i - i.e. "Control4TVGen.c4i"
            max_age - Number of seconds advertisement is valid (e.g. 1800)
            local_only - Whether to broadcast this information only locally

 @return    SDDP_STATUS_SUCCESS on success
            SDDP_STATUS_INVALID_PARAM on failure
*******************************************************************************/
SDDPStatus SDDPSetDevice(const SDDPHandle handle, int index, const char *product_name,
						 const char *primary_proxy, const char *proxies,
						 const char *manufacturer, const char *model,
						 const char *driver, int max_age, bool local_only);

/***************************************************************************//**
 @fn        SDDPStatus SDDPStart(const SDDPHandle handle)

 @brief     Start up the SDDP module. Note the SDDPSetDevice()
            call may be used to alter the device descriptor advertised over
            SDDP later.

 @param     handle - handle to SDDP

 @return    SDDP_STATUS_SUCCESS on success
            SDDP_STATUS_INVALID_PARAM on invalid parameters
            SDDP_STATUS_NETWORK_ERROR - could not initialize or send on network
            SDDP_STATUS_TIME_ERROR - time API not implemented
*******************************************************************************/
SDDPStatus SDDPStart(const SDDPHandle handle);

/***************************************************************************//**
 @fn        SDDPStatus SDDPTick(const SDDPHandle handle)

 @brief     Handles processing of incoming and outgoing SDDP packets. This should 
            be called regularly by the application. This does not block. If a 
            select is used, it can gate the call to this tick based upon the 
            sddp_state->network_info sockets.

 @param     handle - handle to SDDP

 @return    SDDP_STATUS_SUCCESS on success
            SDDP_STATUS_INVALID_PARAM if product information is incorrect
            SDDP_STATUS_NETWORK_ERROR if network is not initialized

*******************************************************************************/
SDDPStatus SDDPTick(const SDDPHandle handle);


/***************************************************************************//**
 @fn        SDDPStatus SDDPIdentify(const SDDPHandle handle)

 @brief     Sends an SDDP identify. This is very similar to an notify, but is 
            typically triggered by a user event, such as a UI or physical button
            push. This sends a packet with the X-SDDPIDENTIFY: TRUE to notify a 
            controller of the identify event on the device.

 @param     handle - handle to SDDP

 @return    SDDP_STATUS_SUCCESS if successful
            SDDP_STATUS_NETWORK_ERROR if network is not initialized or had an error
            
*******************************************************************************/
SDDPStatus SDDPIdentify(const SDDPHandle handle);


/***************************************************************************//**
 @fn        SDDPStatus SDDPLeave(const SDDPHandle handle)

 @brief     Sends byebye but leaves SDDP initialized (i.e. keeps the socket(s) 
            open and continues to send automatic advertisements). This is intended
            to be used prior to shutting down or disconnecting from a project,
            but does not mean the device is no longer present on the network.
            The last is to send a 'byebye' leave notification prior to leaving
            a network. 

 @param     handle - handle to SDDP

 @return    SDDP_STATUS_SUCCESS on success
            SDDP_STATUS_NETWORK_ERROR on nework errors
*******************************************************************************/
SDDPStatus SDDPLeave(const SDDPHandle handle);

/***************************************************************************//**
 @fn        SDDPStatus SDDPShutdown(SDDPHandle handle)

 @brief     Shutdowns the SDDP service, sends byebye, and closes sockets halting
            SDDP automatic advertisments. After calling this, the application 
            must call SDDPInit to reinitialize and the handle is invalid.

 @param     handle - ptr to handle to SDDP, handle will be made invalid

 @return    SDDP_STATUS_SUCCESS on success
            SDDP_STATUS_INVALID_PARAM on invalid parameters
*******************************************************************************/
SDDPStatus SDDPShutdown(SDDPHandle * handle);

#ifdef __cplusplus
}
#endif
#endif
