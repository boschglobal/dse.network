// Copyright 2024 Robert Bosch GmbH
//
// SPDX-License-Identifier: Apache-2.0

#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <dlfcn.h>
#include <dse/testing.h>
#include <dse/logger.h>
#include <dse/network/network.h>
#include <dse/modelc/schema.h>
#include <dse/clib/util/yaml.h>


void network_schedule_reset(Network* net)
{
    if (net->messages == NULL) return;

    if (net->schedule_list) free(net->schedule_list);
    HashList n_list;
    hashlist_init(&n_list, 100);

    for (NetworkMessage* m = net->messages; m->name; m++) {
        if (m->cycle_time_ms) {
            NetworkScheduleItem* ns = calloc(1, sizeof(NetworkScheduleItem));
            ns->message = m;
            hashlist_append(&n_list, ns);
        }
    }
    size_t count = hashlist_length(&n_list);
    net->schedule_list = calloc(count + 1, sizeof(NetworkScheduleItem));
    for (uint32_t i = 0; i < count; i++) {
        memcpy(&net->schedule_list[i], hashlist_at(&n_list, i),
            sizeof(NetworkScheduleItem));
        free(hashlist_at(&n_list, i));
    }
    hashlist_destroy(&n_list);

    /* Reset the tick counter. */
    net->tick = 0;
}


void network_schedule_tick(Network* net)
{
    if (net->schedule_list == NULL) {
        /* Caller forgot to call runnable_schedule_reset()? */
        log_error("No scheduled tasks!");
        net->tick++; /* Tick so caller does not hang. */
        return;
    }
    /* Current tick. */
    for (NetworkScheduleItem* s = net->schedule_list; s->message; s++) {
        if (net->tick == 0) {
        } else {
            /* Decrement the alarm counter. */
            if (s->alarm) {
                s->alarm--;
                /* Transition to 0? */
                if (s->alarm == 0) {
                    /* Alarm fired, reset checksum.*/
                    if (s->message->cycle_time_ms) {
                        s->message->buffer_checksum = 0;
                        s->message->needs_tx = true;
                    }
                }
            }
        }
        /* Reset the alarm counter. */
        if (s->alarm == 0) {
            s->alarm = s->message->cycle_time_ms;
        }
    }

    /* Next tick. */
    net->tick++;
}
