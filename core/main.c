#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "arg.h"
#include "event.h"
#include "conn.h"
#include "event.h"
#include "net_pull.c"
#include "net_handle.h"
#include "net_utils.h"
#include "net_http.h"

#include "main.h"

enum main_stat_e main_stat;

static int proxy_done(int exit_val) {
    PLOGD("Exiting");
    main_stat = EXITING;
    net_pull_done();
    net_data_module_done();
    conn_module_done();
    event_module_done();
    proxy_log_done();
    exit(exit_val);
    return 0;
}

static void sig_handler(int sig) {
    PLOGI("Recving signal %d: %s", sig, strsignal(sig));
    proxy_done(-1);
}

static void install_signal_handlers() {
    signal(SIGTERM, sig_handler);
    signal(SIGINT, sig_handler);
    //signal(SIGKILL, sig_handler);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGTERM, sig_handler);
    signal(SIGSEGV, sig_handler);
    signal(SIGHUP, sig_handler);
}

int proxy_main(int argc, char** argv) {
    main_stat = RUNING;
    
    proxy_log_init();
    PLOGD("Starting");

    install_signal_handlers();

    int ret = proxy_parse_arg(argc, argv);
    if (ret) {
        PLOGD("Cannot parse arguments. Aborting");
        return 0;
    }

    net_data_module_init();
    event_module_init();

    ret = net_pull_init();
    if (ret) {
        PLOGD("Cannot init epoll. Aborting");
        goto proxy_main_exit;
    }
    
    net_utils_init();

    ret = conn_module_init();
    if (ret) {
        PLOGD("Cannot init connection module. Aborting");
        goto proxy_main_exit;
    }

    
    net_handle_module_init();
    net_http_module_init();

    while (1) {
        ret = event_work();
        if (ret) 
            break;
        ret = net_pull_work();
        if (ret)
            break;
//        sleep(1);
    }

proxy_main_exit:
    proxy_done(ret);
    return 0;
}
