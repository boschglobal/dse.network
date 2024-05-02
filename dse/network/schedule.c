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


void network_schedule_reset(Network* n)
{
    if (n->messages == NULL) return;

    if (n->schedule_list) free(n->schedule_list);
    HashList n_list;
    hashlist_init(&n_list, 100);

    for (NetworkMessage* nm = n->messages; nm && nm->name; nm++) {
        if (nm->cycle_time_ms) {
            NetworkScheduleItem* nsi = calloc(1, sizeof(NetworkScheduleItem));
            nsi->message = nm;
            hashlist_append(&n_list, nsi);
        }
    }
    size_t count = hashlist_length(&n_list);
    n->schedule_list = calloc(count + 1, sizeof(NetworkScheduleItem));
    for (uint32_t i = 0; i < count; i++) {
        memcpy(&n->schedule_list[i], hashlist_at(&n_list, i),
            sizeof(NetworkScheduleItem));
        free(hashlist_at(&n_list, i));
    }
    hashlist_destroy(&n_list);

    /* Reset the tick counter. */
    n->tick = 0;
}


void network_schedule_tick(Network* n)
{
    if (n->schedule_list == NULL) {
        /* Caller forgot to call runnable_schedule_reset()? */
        log_error("No scheduled tasks!");
        n->tick++; /* Tick so caller does not hang. */
        return;
    }
    /* Current tick. */
    for (NetworkScheduleItem* nsi = n->schedule_list; nsi && nsi->message;
         nsi++) {
        if (n->tick == 0) {
        } else {
            /* Decrement the alarm counter. */
            if (nsi->alarm) {
                nsi->alarm--;
                /* Transition to 0? */
                if (nsi->alarm == 0) {
                    /* Alarm fired, reset checksum.*/
                    if (nsi->message->cycle_time_ms) {
                        nsi->message->buffer_checksum = 0;
                        nsi->message->needs_tx = true;
                    }
                }
            }
        }
        /* Reset the alarm counter. */
        if (nsi->alarm == 0) {
            nsi->alarm = nsi->message->cycle_time_ms;
        }
    }

    /* Next tick. */
    n->tick++;
}
