/*
 * Copyright (C) 2017 Matthias Ringwald
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY MATTHIAS RINGWALD AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MATTHIAS
 * RINGWALD OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/**
 * Implementation of ATV Power State Library
 */

#include "atv.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef WIN32
#include <windows.h>
#define sleep(x) Sleep(x*1000)
#endif

#include <libimobiledevice/libimobiledevice.h>
#include <libimobiledevice/syslog_relay.h>

static void (*update_handler)(int state);

static int quit_flag = 0;

static char* udid = NULL;

static idevice_t device = NULL;
static syslog_relay_client_t syslog = NULL;

static int old_stuff;

static char line_buffer[200];
static int line_pos = 0;

// suffix indicates tvOS version
static const char * ACTIVATE_SLEEP_TAG_12 = "Blocking events on the way down to sleep";
static const char * ACTIVATE_SLEEP_TAG_13 = "Clearing boot count prior to sleep";
static const char * ACTIVATE_SLEEP_TAG_16 = "Releasing power source";

static const char * SIRI_TAG = "usagePage:0xc usage:0x4 downEvent:1";
static const char * SIRI_TAG_16 = "page:0xC usage:0x4 downEvent:1";

static int find_tag(const char * tag){
	int i;
	int tag_len = strlen(tag);
	for (i=0;i<line_pos-tag_len;i++){
		if (strncmp(tag, &line_buffer[i], tag_len) == 0){
			printf("Found tag '%s' at pos %u\n", tag, i);
			return 1;
		}
	}
	return 0;
}

static void process_line(void){
	if (find_tag(ACTIVATE_SLEEP_TAG_12)){
		update_handler(0);
	}
	if (find_tag(ACTIVATE_SLEEP_TAG_13)){
		update_handler(0);
	}
	if (find_tag(ACTIVATE_SLEEP_TAG_16)){
		update_handler(0);
	}
	if (find_tag(SIRI_TAG)){
		update_handler(1);
	}
	if (find_tag(SIRI_TAG_16)){
		update_handler(1);
	}
	line_pos = 0;
}

static void syslog_callback(char c, void *user_data)
{
	if (old_stuff) return;

	if (c == '\r' || c == '\n'){
		line_buffer[line_pos] = 0;
		process_line();
		return;
	}

	if (line_pos >= sizeof(line_buffer)-1) return;

	line_buffer[line_pos++] = c;
}

static int start_logging(void)
{
	line_pos = 0;
	idevice_error_t ret = idevice_new(&device, udid);
	if (ret != IDEVICE_E_SUCCESS) {
		fprintf(stderr, "Device with udid %s not found!?\n", udid);
		return -1;
	}

	/* start and connect to syslog_relay service */
	syslog_relay_error_t serr = SYSLOG_RELAY_E_UNKNOWN_ERROR;
	serr = syslog_relay_client_start_service(device, &syslog, "idevicesyslog");
	if (serr != SYSLOG_RELAY_E_SUCCESS) {
		fprintf(stderr, "ERROR: Could not start service com.apple.syslog_relay.\n");
		idevice_free(device);
		device = NULL;
		return -1;
	}

	/* start capturing syslog */
	serr = syslog_relay_start_capture(syslog, syslog_callback, NULL);
	if (serr != SYSLOG_RELAY_E_SUCCESS) {
		fprintf(stderr, "ERROR: Unable tot start capturing syslog.\n");
		syslog_relay_client_free(syslog);
		syslog = NULL;
		idevice_free(device);
		device = NULL;
		return -1;
	}

	fprintf(stdout, "[connected]\n");
	fflush(stdout);

	return 0;
}

static void stop_logging(void)
{
	fflush(stdout);

	if (syslog) {
		syslog_relay_client_free(syslog);
		syslog = NULL;
	}

	if (device) {
		idevice_free(device);
		device = NULL;
	}
}

static void device_event_cb(const idevice_event_t* event, void* userdata)
{
	if (event->event == IDEVICE_DEVICE_ADD) {
		if (!syslog) {
			if (!udid) {
				udid = strdup(event->udid);
			}
			if (strcmp(udid, event->udid) == 0) {
				if (start_logging() != 0) {
					fprintf(stderr, "Could not start logger for udid %s\n", udid);
				}
			}
		}
	} else if (event->event == IDEVICE_DEVICE_REMOVE) {
		if (syslog && (strcmp(udid, event->udid) == 0)) {
			stop_logging();
			fprintf(stdout, "[disconnected]\n");
		}
	}
}

void atv_init(void (*handler)(int new_state)){

	int i;

	update_handler = handler;

	int num = 0;
	char **devices = NULL;
	idevice_get_device_list(&devices, &num);
	idevice_device_list_free(devices);
	if (num == 0) {
		fprintf(stderr, "No device found. Plug in a device or pass UDID with -u to wait for device to be available.\n");
		return;
	}

	idevice_event_subscribe(device_event_cb, NULL);

	printf("Skipping old stuff\n");
	old_stuff = 1;
	sleep(5);
	old_stuff = 0;
	printf("Live...\n");
}

void atv_stop(void){

	idevice_event_unsubscribe();
	stop_logging();

	if (udid) {
		free(udid);
	}
}
