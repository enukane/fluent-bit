/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*  Fluent Bit
 *  ==========
 *  Copyright (C) 2015 Treasure Data Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/event.h>

#include <fluent-bit/flb_macros.h>
#include <fluent-bit/flb_input.h>
#include <fluent-bit/flb_output.h>
#include <fluent-bit/flb_error.h>
#include <fluent-bit/flb_utils.h>
#include <fluent-bit/flb_config.h>
#include <fluent-bit/flb_network.h>
#include <fluent-bit/flb_engine.h>

#define FLB_TIMER_FD 0xDEADBEAF

static int timer_fd(int kq, time_t sec, long nsec)
{
    int ret;
    int64_t millisec = 0;
    static int count = 0;
    int fd;
    static struct kevent kev;

    fd = FLB_TIMER_FD + count;
    count++;

    millisec = sec * 1000 + nsec / (1000 * 1000);

    EV_SET(&kev, fd, EVFILT_TIMER, EV_ADD, 0, millisec, 0);
    ret = kevent(kq, &kev, 1, NULL, 0, NULL);

    return fd;
}

static int flb_engine_loop_create()
{
    int efd;

    efd = kqueue();
    if (efd == -1) {
        perror("kqueue");
        return -1;
    }

    return efd;
}

static int flb_engine_loop_add(int efd, int fd, int mode)
{
    int ret;
    struct kevent kev;

    EV_SET(&kev, fd, EVFILT_READ, EV_ADD, 0, 0, 0);
    ret = kevent(efd, &kev, 1, NULL, 0, NULL);
    if (ret == -1) {
        perror("kevent");
        return -1;
    }

    return ret;
}

int flb_engine_flush(struct flb_config *config,
                     struct flb_input_plugin *in_force,
                     struct flb_output_plugin *tmp)
{
    int fd;
    int size;
    int len;
    int bytes;
    char *buf;
    struct flb_input_plugin *in;
    struct flb_output_plugin *out;
    struct iovec *iov;
    struct mk_list *head;

    if (!tmp) {
        out = mk_list_entry_first(&config->outputs,
                                  struct flb_output_plugin, _head);
    }
    else {
        out = tmp;
    }

    /*
     * Lazy flush: it does a connect in blocking mode, this needs
     * to be changed later and be integrated with the main loop.
     */
    fd = flb_net_tcp_connect(out->host, out->port);
    if (fd == -1) {
        flb_error("Error connecting to output service");
        return -1;
    }

    mk_list_foreach(head, &config->inputs) {
        in = mk_list_entry(head, struct flb_input_plugin, _head);

        if (in_force != NULL && in != in_force) {
            continue;
        }

        if (in->active == FLB_TRUE) {
            if (in->cb_flush_buf) {
                buf = in->cb_flush_buf(in->in_context, &size);
                if (!buf) {
                    goto flush_done;
                }

                bytes = config->output->cb_flush(buf, size,
                                                 config->output->out_context);
                if (bytes <= 0) {
                    flb_error("Error flushing data");
                }
                else {
                    flb_info("Flush buf %i bytes", bytes);
                }
                free(buf);
            }

            if (in->cb_flush_iov) {
                iov = in->cb_flush_iov(in->in_context, &len);
                if (len <= 0) {
                    goto flush_done;
                }

                bytes = writev(fd, iov, len);
                if (bytes <= 0) {
                    perror("writev");
                }
                else {
                    flb_info("Flush iov %i bytes (%i entries)", bytes, len);
                }
            }

        flush_done:
            if (in->cb_flush_end) {
                in->cb_flush_end(in->in_context);
            }
        }
    }
    close(fd);
    return 0;
}

static inline int consume_byte(int fd)
{
    int ret;
    uint64_t val;

    /* We need to consume the byte */
    ret = read(fd, &val, sizeof(val));
    if (ret <= 0) {
        perror("read");
        return -1;
    }

    return 0;
}

static int flb_engine_handle_event(int fd, int mask, struct flb_config *config)
{
    struct mk_list *head;
    struct flb_input_collector *collector;

    if (mask == FLB_ENGINE_READ) {
        /* Check if we need to flush */
        if (config->flush_fd == fd) {
            consume_byte(fd);
            flb_engine_flush(config, NULL, config->output);
            return 0;
        }

        /* Determinate what is this file descriptor */
        mk_list_foreach(head, &config->collectors) {
            collector = mk_list_entry(head, struct flb_input_collector, _head);
            if (collector->fd_event == fd) {
                return collector->cb_collect(config,
                                             collector->plugin->in_context);
            }
            else if (collector->fd_timer == fd) {
                consume_byte(fd);
                return collector->cb_collect(config,
                                             collector->plugin->in_context);
            }
        }
    }

    return -1;
}

int flb_engine_start(struct flb_config *config)
{
    int i;
    int fd;
    int ret;
    int mask;
    int loop; /* kq */
    int n;
    int size = 64;
    struct kevent kev;
    struct mk_list *head;
    struct flb_input_collector *collector;

    flb_info("starting engine");

    /* Inputs pre-run */
    flb_input_pre_run_all(config);

    /* Outputs pre-run */
    flb_output_init(config);
    flb_output_pre_run(config);

    /* main loop */
    loop = flb_engine_loop_create();
    if (loop == -1) {
        return -1;
    }

    /* Create and register the timer fd for flush procedure */
    config->flush_fd = timer_fd(loop, config->flush, 0);
    if (config->flush_fd == -1) {
        flb_utils_error(FLB_ERR_CFG_FLUSH_CREATE);
    }

    /* For each Collector, register the event into the main loop */
    mk_list_foreach(head, &config->collectors) {
        collector = mk_list_entry(head, struct flb_input_collector, _head);

        if (collector->type == FLB_COLLECT_TIME) {
            fd = timer_fd(loop, collector->seconds, collector->nanoseconds);
            if (fd == -1) {
                continue;
            }
            collector->fd_timer = fd;
        }
        else if (collector->type & (FLB_COLLECT_FD_EVENT | FLB_COLLECT_FD_SERVER)) {
            ret = flb_engine_loop_add(loop, collector->fd_event,
                                      FLB_ENGINE_READ);
            if (ret == -1) {
                close(fd);
                continue;
            }
        }
    }

    while (1) {
        n = kevent(loop, NULL, 0, &kev, 1, NULL);
        flb_engine_handle_event(kev.ident, FLB_ENGINE_READ, config);
    }
}
