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
 * Example for ATV Power State Library
 */

#include "atv.h"
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>

#ifdef HAVE_LIBCURL
#include <curl/curl.h>
#include <stdlib.h>

#endif

static int quit_flag = 0;

static char const * udid;
static int use_network;

#ifdef HAVE_LIBCURL
static char short_options[] = "hu:n1:0:";
#else
static char short_options[] = "hu:n";
#endif

#ifdef HAVE_LIBCURL
static char const * on_url;
static char const * off_url;
#endif

static struct option long_options[] = {
        {"help", no_argument,       NULL,   'h'},
        {"udid", required_argument, NULL,   'u'},
        {"network", no_argument, NULL,      'n'},
#ifdef HAVE_LIBCURL
        {"on",   required_argument, NULL,   '1'},
        {"off",  required_argument, NULL,   '0'},
#endif
        {0, 0, 0, 0}
};

static char *help_options[] = {
        "print (this) help.",
        "target specific device by UDID",
        "connect to network device",
#ifdef HAVE_LIBCURL
        "HTTP GET request when Apple TV is turned on.",
        "HTTP GET request when Apple TV is turned off.",
#endif
};

static char *option_arg_name[] = {
        "",
        "UDID",
        "",
#ifdef HAVE_LIBCURL
        "URL",
        "URL",
#endif
};

static void usage(const char *name){
    unsigned int i;
    printf( "usage:\n\t%s [options]\n", name );
    printf("valid options:\n");
    for( i=0; long_options[i].name != 0; i++) {
        printf("--%-10s| -%c  %-10s\t\t%s\n", long_options[i].name, long_options[i].val, option_arg_name[i], help_options[i] );
    }
}

#ifdef HAVE_LIBCURL
static void get_request(const char * url){
    CURL *curl;
    CURLcode res;

    printf("HTTP GET Request: %s\n", url);
    if (url == NULL){
        return;
    }

    // from curl/simple.c
    curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }
        curl_easy_cleanup(curl);
    }
}
#endif

static void update_handler(int state){
	printf("ATV update, new state %u\n", state);
#ifdef HAVE_LIBCURL
    get_request(state ? on_url : off_url);
#endif
}

static void clean_exit(int sig)
{
	fprintf(stderr, "\nExiting...\n");
	quit_flag++;
}

int main(int argc, char * argv[]){

	signal(SIGINT, clean_exit);
	signal(SIGTERM, clean_exit);
#ifndef WIN32
	signal(SIGQUIT, clean_exit);
	signal(SIGPIPE, SIG_IGN);
#endif

    while(true){
        int c = getopt_long( argc, (char* const *)argv, short_options, long_options, NULL );
        if (c < 0) {
            break;
        }
        if (c == '?'){
            break;
        }
        switch (c) {
            case 'u':
                if (!*optarg) {
                    fprintf(stderr, "ERROR: UDID must not be empty!\n");
                    usage(argv[0]);
                    return EXIT_FAILURE;
                }
                break;
            case 'n':
                use_network = 1;
                break;
#ifdef HAVE_LIBCURL
            case '1':
                on_url = optarg;
                break;
            case '0':
                off_url = optarg;
                break;
#endif
            case 'h':
            default:
                usage(argv[0]);
                return EXIT_FAILURE;
        }
    }

    atv_init(&update_handler, use_network, udid);
	while (!quit_flag){
		sleep(1);
	}
	atv_stop();
}
