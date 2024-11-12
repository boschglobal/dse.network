---
title: Example Network Function API Reference
linkTitle: Functions
---
## Example Network Functions


Example implementation of Network Functions.



## Typedefs

### InstanceData

```c
typedef struct InstanceData {
    int position;
}
```

## Functions

### counter_inc_uint8

Increment an 8-bit counter in the message packet.

> Note: in the encode path, changes to the counter are not reflected in
the corresponding signal. Subsequent calls to `network_message_recalculate`
may overwrite the modified counter.

#### Parameters

data (void**)
: Pointer reference for instance data.

payload (uint8_t*)
: The payload that this function will modify.

payload_len (size_t)
: The length of the payload.


#### Returns

0
: Counter incremented.

ENOMEM
: Instance data could not be established.

EPROTO
: A required annotation was not located.

#### Annotations

position
: The position of the counter in the message packet.
 


### crc_generate

Calculate a CRC based on the message packet. The CRC is written into the
specified position in the message packet.

The CRC algorithm is a simple summation of all bytes in the message packet.

> Note: in the encode path (TX), changes to the counter are not reflected in
the corresponding signal.

#### Parameters

data (void**)
: Pointer reference for instance data.

payload (uint8_t*)
: The payload that this function will modify.

payload_len (size_t)
: The length of the payload.

#### Returns

0
: CRC generated.

ENOMEM
: Instance data could not be established.

EPROTO
: A required annotation was not located.

#### Annotations

position
: The position of the CRC in the message packet.
 


### crc_validate

Validate the CRC of a message packet. The CRC is included in the message packet.

> Note: in the decode path (RX), bad messages (function returns EBADMSG) will
not change corresponding signals.

#### Parameters

data (void**)
: Pointer reference for instance data.

payload (uint8_t*)
: The payload that this function will modify.

payload_len (size_t)
: The length of the payload.

#### Returns

0
: The CRC passed validation.

EBADMSG
: The CRC failed validation. The message will not be decoded.

ENOMEM
: Instance data could not be established.

EPROTO
: A required annotation was not located.

#### Annotations

position
: The position of the CRC in the message packet.
 


