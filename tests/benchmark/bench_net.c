
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <time.h>
#include <unistd.h>


struct timespec get_timespec_now(void)
{
    struct timespec ts = {};
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts;
}


uint64_t get_elapsedtime_ns(struct timespec ref)
{
    struct timespec now = get_timespec_now();
    if (ref.tv_sec == now.tv_sec) {
        return now.tv_nsec - ref.tv_nsec;
    } else {
        return ((now.tv_sec - ref.tv_sec) * 1000000000) +
               (now.tv_nsec - ref.tv_nsec);
    }
}

double ns_to_us_to_sec(uint64_t t_ns)
{
    uint32_t t_us = t_ns / 1000;
    return t_us / 1000000.0;
}

typedef int (*PackFunc)(uint8_t*, const void*, size_t);
typedef int (*UnpackFunc)(void*, const uint8_t*, size_t);
typedef bool (*RangeFunc)(uint8_t value);
typedef double (*DecodeFunc)(uint8_t value);
typedef uint8_t (*EncodeFunc)(double signal);


typedef struct signal_t {
    int        count;
    PackFunc   pack_func;
    UnpackFunc unpack_func;
    RangeFunc  range_func;
    DecodeFunc decode_func;
    EncodeFunc encode_func;

    uint8_t buffer[8];
    uint8_t pack[16];
} signal_t;


void* _get_func_handle(void* handle, const char* fmt, int index)
{
    char b[1000];
    snprintf(b, 1000, fmt, index);
    // printf("  loading function : %s\n", b);
    void* func = dlsym(handle, b);
    if (func == NULL) {
        printf("Could not load function from library! (%s)", dlerror());
        exit(1);
    }
    return func;
}


void run_bench_ct(double* signals, signal_t* st, int count, int steps)
{
    struct timespec _ts = get_timespec_now();

    for (int step = 0; step < steps; step++) {
        for (int i = 0; i < count; i++) {
            // Encode
            double original = signals[i];
            double value = original + 1;
            // printf("  signal[%d] val=%f (orig=%f) .. ", i, value, original);
            if (value > 100) value = 0.0;
            st[i].buffer[0] = st[i].encode_func(value);
            st[i].pack_func(st[i].pack, st[i].buffer, 8);
            // Decode
            st[i].buffer[0] = 0;
            st[i].unpack_func(st[i].buffer, st[i].pack, 8);
            if (st[i].range_func(st[i].buffer[0])) {
                value = st[i].decode_func(st[i].buffer[0]);
            } else {
                value = value + 1;
                // printf(" .. out of range .. ");
            }
            // printf("val=%f (orig=%f)\n", value, original);
            signals[i] = value;
        }
    }

    uint64_t time_ns = get_elapsedtime_ns(_ts);
    printf("CANtools: Time %.9f (steps=%d, signals=%d)\n",
        ns_to_us_to_sec(time_ns), steps, count);
}


void run_bench_loop(double* signals, signal_t* st, int count, int steps)
{
    struct timespec _ts = get_timespec_now();

    for (int step = 0; step < steps; step++) {
        for (int i = 0; i < count; i++) {
            // Encode
            double original = signals[i];
            double value = original + 1;
            if (value > 100) value = 0.0;
            st[i].buffer[0] = (uint8_t)(value / 0.5);
            // memset(st[i].pack, 0, 8);
            st[i].pack[0] |=
                (uint8_t)((uint8_t)(st[i].buffer[0] << 1u) & 0x7eu);
            // Decode
            st[i].buffer[0] = (uint8_t)((uint8_t)(st[i].pack[0] & 0x7eu) >> 1u);
            ;
            if (st[i].buffer[0] <= 200u) {
                value = (double)st[i].buffer[0] * 0.5;
            } else {
                value = value + 1;
                // printf(" .. out of range .. ");
            }
            // printf("val=%f (orig=%f)\n", value, original);
            signals[i] = value;
        }
    }

    uint64_t time_ns = get_elapsedtime_ns(_ts);
    printf("LOOP:     Time %.9f (steps=%d, signals=%d)\n",
        ns_to_us_to_sec(time_ns), steps, count);
}


typedef struct vector_t {
    int count;

    double*  signal;
    double*  factor;
    int64_t* offset;

    uint64_t* buffer;
    uint64_t* pack;

    uint64_t* max;
    uint64_t* min;
    uint64_t* shift;
    uint64_t* mask;
} vector_t;

void _allocate_vectors(vector_t* v)
{
    v->signal = calloc(v->count, sizeof(double));
    v->factor = calloc(v->count, sizeof(double));
    v->offset = calloc(v->count, sizeof(int64_t));

    v->buffer = calloc(v->count, sizeof(uint64_t));
    v->pack = calloc(v->count, sizeof(uint64_t));

    v->max = calloc(v->count, sizeof(uint64_t));
    v->min = calloc(v->count, sizeof(uint64_t));
    v->shift = calloc(v->count, sizeof(uint64_t));
    v->mask = calloc(v->count, sizeof(uint64_t));
}

void run_bench_vector(double* signals, signal_t* st, int count, int steps)
{
    vector_t v = { .count = count };
    _allocate_vectors(&v);
    struct timespec _ts = get_timespec_now();

    for (int step = 0; step < steps; step++) {
        for (int i = 0; i < count; i++) {
            double original = v.signal[i];
            double value = original + 1;
            value = value > 100 ? 0.0 : value;
            value = value / 0.5;
            v.buffer[i] = (uint64_t)((uint8_t)(value));
            v.pack[i] |=
                (uint64_t)((uint8_t)((uint8_t)(st[i].buffer[0] << 1u) & 0x7eu));
            v.buffer[i] =
                (uint64_t)((uint8_t)((uint8_t)(v.pack[i] & 0x7eu) >> 1u));
            if (v.buffer[i] <= 200u) {
                value = v.buffer[i] * 0.5;
            } else {
                value = value + 1;
            }
            v.signal[i] = value;
        }
    }

    uint64_t time_ns = get_elapsedtime_ns(_ts);
    printf("VECTOR:   Time %.9f (steps=%d, signals=%d)\n",
        ns_to_us_to_sec(time_ns), steps, count);
}


int main(int argc, char** argv)
{
    if (argc != 2) {
        printf("Incorrect arguments!");
        exit(1);
    }
    int signalCount = argv[1] ? atoi(argv[1]) : 0;
    int steps = 10;

    printf("Running Network CANtools benchmark\n");
    printf("signals : %d\n", signalCount);

    void* handle = dlopen("build/network_ct.so", RTLD_NOW | RTLD_GLOBAL);
    if (handle == NULL) {
        printf("Could not open message library! (%s)", dlerror());
        exit(1);
    }

    printf("  loading signal functions ...\n");
    signal_t* signal_table = calloc(signalCount, sizeof(signal_t));
    for (int i = 0; i < signalCount; i++) {
        signal_table[i].pack_func =
            _get_func_handle(handle, "message%d_pack", i);
        signal_table[i].unpack_func =
            _get_func_handle(handle, "message%d_unpack", i);
        signal_table[i].range_func =
            _get_func_handle(handle, "message%d_signal_is_in_range", i);
        signal_table[i].decode_func =
            _get_func_handle(handle, "message%d_signal_decode", i);
        signal_table[i].encode_func =
            _get_func_handle(handle, "message%d_signal_encode", i);
    }

    double* signals = calloc(signalCount, sizeof(double));
    for (int i = 0; i < signalCount; i++) {
        signals[i] = i % 100;
    }
    printf("  run cantools based benchmark ...\n");
    run_bench_ct(signals, signal_table, signalCount, steps);
    run_bench_loop(signals, signal_table, signalCount, steps);
    run_bench_vector(signals, signal_table, signalCount, steps);


    exit(0);
}
