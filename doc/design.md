


signals [index count]

    double      signal[]
    uint64_t    value[]     // value (encoded, to be packed)
    double      factor[]    // encoding factor
    int64_t     offset[]    // encoding offset
    uint64_t    min[]       // encoding min (or apply at signal)
    uint64_t    max[]       // encoding max (or apply at signal)
    uint64_t    shift[]     // packing shift
    uint64_t    mask[]      // packing mask
    uint8_t     endian[]    // packing algo
    uint8_t     type[]      // type of value (uint8_t ... float, double, map)
    hash        map{value -> value ??}

messages [index count]

    uint64_t    offset[]    // offset to signals
    uint64_t    count[]     // count of signals (in this message)
    uint8_t     packed[]    // packed message
    uint8_t     rx_update[] // indicate rx (or use checksum)
