/*-
 * Copyright (c) 2014      Rick Ratzel,
 *                         National Instruments.
 * All Rights Reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef PTPAPI_H_
#define PTPAPI_H_

#include <stdio.h>

/*
 * Include guard for ptpd types - if ptpd.h was included before, do not
 * redeclare these types.
 *
 * TODO: move these to a shared header.
 */
#ifndef PTPD_H_

typedef int32_t Integer32;
typedef uint8_t UInteger8;
typedef uint16_t UInteger16;
typedef unsigned char Enumeration8;

enum {
   PTP_INITIALIZING=1,  PTP_FAULTY,  PTP_DISABLED,
   PTP_LISTENING,  PTP_PRE_MASTER,  PTP_MASTER,
   PTP_PASSIVE,  PTP_UNCALIBRATED,  PTP_SLAVE
};

typedef struct {
   UInteger8 clockClass;
   Enumeration8 clockAccuracy;
   UInteger16 offsetScaledLogVariance;
} ClockQuality;

typedef void (*ClockAdjustFunc)( Integer32 oldSeconds, Integer32 oldNanoseconds,
                                 Integer32 newSeconds, Integer32 newNanoseconds,
                                 void* clientData );

#endif /* PTPD_H_ */


/*
 * Forward declarations for ptpd structs used in the API.  These are fully
 * defined in ptp_api.c and dep/iniparser/dictionary.h
 */
typedef struct PtpSession PtpSession;
typedef struct _dictionary_ PtpDictionary;
typedef struct PtpManagedSettings PtpManagedSettings;


/*
 * TODO: Reconcile the return code convention across all of ptpd when
 * int is the return type. The ptp_api functions return 0 on success while
 * the internal ptpd functions return 1 on success and 0 or -1 on failure.
 */


/*
 * Allocate memory for and initialize a new, unique ptpd session. "session" will
 * be assigned the pointer to the new session, which can be freed using
 * ptp_deleteSession().
 *
 * Returns 0 on success and ENOMEM (Out of memory) if memory could not be
 * allocated and EINVAL if "session" is NULL.
 */
int ptp_initializeSession( PtpSession** session );


/*
 * Free the memory allocated by ptp_initializeSession().
 *
 * Returns 0 on success or EINVAL if "session" is NULL.
 */
int ptp_deleteSession( PtpSession* session );


/*
 * Set the runtime options for a ptpd session based on the values of argv.  argc
 * is the number of arguments in argv.
 *
 * shouldRun is set to 1 if the arguments are sufficient to allow the PTP
 * protocol to run, and 0 if PTP should not be run.  For example, this is useful
 * if the argv arguments contain options for checking configuration and not
 * necessarily starting the protocol.
 *
 * Returns 0 on successful parsing and application of runtime options from argv,
 * and non-zero if argv could not be parsed or resulted in invalid runtime
 * options.
 */
int ptp_setOptsFromCommandLine( PtpSession* session,
                                int argc, char** argv,
                                int* shouldRun );


/*
 * Test that the network configuration is valid
 *
 * Returns 0 on success and non-zero on failure.
 */
int ptp_testNetworkConfig( PtpSession* session );


/*
 * Return 1 if the ptpd session was configured to check the configuration
 * options only, and 0 if configured to run the PTP protocol.
 */
int ptp_checkConfigOnly( PtpSession* session );


/*
 * Prints the current configuration of the ptpd session to an open file handle.
 * STDIO and STDERR are acceptable file handles.
 *
 * Returns 0 on successful write and EOPNOTSUPP if the configuration could not
 * be written.
 */
int ptp_dumpConfig( PtpSession* session, FILE* out );


/*
 * Duplicates the configuration settings of a ptpd session to a new config
 * object and assigns the "config" pointer to it.  Any existing settings in
 * "config" will be deleted, and "config" must eventually be deleted using
 * ptp_deleteConfigObject().
 *
 * Returns 0 on successful duplication, ENODATA if the ptpd session did not have
 * a current configuration, and ENOMEM if new config object contents could not
 * be allocated.
 */
int ptp_duplicateConfig( PtpSession* session, PtpDictionary** config );


/*
 * Deletes the contents of a config object created by ptp_duplicateConfig(),
 * leaving it in a state that can be either re-used by another call to
 * ptp_duplicateConfig() or discarded.
 *
 * Returns 0 on success or EINVAL if "config" is NULL.
 */
int ptp_deleteConfigObject( PtpDictionary* config );


/*
 * Applies a configuration from a config object to the ptpd session by
 * completely removing it and replacing with settings from "config".  This
 * operation can only be done if "session" is not running the protocol.
 *
 * Returns 0 on success, EBUSY if "session" is currently running the protocol,
 * and ENOMEM if a new config could not be allocated in the session.
 */
int ptp_applyConfigObject( PtpSession* session, PtpDictionary* config );


/*
 * Runs the PTP protocol using the configuration state set in the ptpd session.
 * This function does not return until the protocol is stopped.  If a non-NULL
 * PtpManagedSettings** is passed, upon return it will contain values that may
 * have been set by 1588 management messages while the protocol was running.
 * This allows the caller to re-run the protocol with a previous state.  Passing
 * NULL for PtpManagedSettings** results in no attempt to save the managed
 * settings.
 *
 * Returns 0 if the protocol was successfuly run and stopped, ENOMEM if memory
 * could not be allocated for a new PtpManagedSettings struct, and non-zero if
 * stopped due to an error.
 */
int ptp_run( PtpSession* session, PtpManagedSettings** settings );


/*
 * Deletes a PtpManagedSettings struct allocated by ptp_run().
 *
 * Returns 0 on success and EINVAL if settings is NULL.
 */
int ptp_deleteManagedSettingsObject( PtpManagedSettings* settings );


/*
 * Stops the PTP protocol running which is part of the ptpd session.  If the
 * ptpd session is running the event timer in a separate thread, this will
 * properly stop the protocol and cause the call to ptp_run() to return.
 *
 * Return 0 on successful stop of the protocol, EOPNOTSUPP (Operation not
 * supported) if the ptpd session was not configured to run the event timer in a
 * separate thread, EAGAIN if the event timer is not running, and EINVAL if the
 * session was not created.  If the ptpd session is running the event timer in a
 * separate thread and the protocol has not stopped after 10 seconds, EBUSY is
 * returned.
 */
int ptp_stop( PtpSession* session );


/*
 * Causes the ptpd session to run the PTP protocol event timer in a separate
 * thread.  The default behavior for a ptpd session is to use signal-based
 * timers for protocol events.
 *
 * A timer thread is needed if a ptpd session is to be created as part of a
 * larger threaded application, since the signal-based event timers require
 * signal handlers to be implemented in the main thread and have access to ptpd
 * internals, which may not be an attractive option depending on the
 * application.
 */
int ptp_enableTimerThread( PtpSession* session );


/*
 * Registers an optional callback function on the ptpd session that will be
 * called to adjust a client-specific servo/timekeeper instead of the default
 * ptpd servo/system timekeeper.
 *
 * If a ClockAdjustFunc callback is supplied, ptpd will call it *instead of*
 * calling the ptpd servo to adjust the system time.  Because the callback
 * bypasses the ptpd servo, it is also responsible for servo-ing the
 * client-specific timekeeper.
 *
 * ClockAdjustFunc is a function pointer defined in ptp_api.h, and clientData is
 * an optional argument that ptpd will pass to the client callback if provided.
 *
 * Returns 0 if the callback was successfully registered and EBUSY (Device or
 * resource busy) if the ptpd session is currently running the PTP protocol.
 */
int ptp_setClockAdjustFunc( PtpSession* session,
                            ClockAdjustFunc clientClockAdjustFunc, void* clientData );


/*
 * TODO: implement this function.
 *
 * Sets the outgoing clock quality values used by the ptpd session to the values
 * set in the ClockQuality struct.
 *
 * Returns 0 on successful set of clock quality values, and non-zero otherwise.
 */
int ptp_setClockQuality( PtpSession* session,
                         ClockQuality* newClockQuality );


/*
 * Sets the clock quality values in the ptpd session to that of the grandmaster
 * the session is synchronized to.
 *
 * Returns 0 if successfully set and ENOPROTOOPT (Protocol not available) if the
 * ptpd session is not currently running the PTP protocol.
 */
int ptp_getGrandmasterClockQuality( PtpSession* session,
                                    ClockQuality* gmClockQuality );


/*
 * Sets the port state value using the ptpd session.
 *
 * Returns 0 if successfully set and ENOPROTOOPT (Protocol not available) if the
 * ptpd session is not currently running the PTP protocol.
 */
int ptp_getPortState( PtpSession* session,
                      Enumeration8* portState );


/*
 * Gets the network interface name using the ptpd session.
 *
 * Returns 0 if successfully set and ENODATA (No data available) if the
 * ptpd session does not have it available.
 */
int ptp_getInterfaceName( PtpSession* session,
                          char** interface );


/*
 * Logs an error message using the ptpd logging mechanism.
 */
void ptp_logError( const char* msg );


/*
 * Logs a notification message using the ptpd logging mechanism.
 */
void ptp_logNotify( const char* msg );


#endif /* PTPAPI_H_ */