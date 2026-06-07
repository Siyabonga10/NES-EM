#ifndef SAVE_STATE_PACKED_H
#define SAVE_STATE_PACKED_H

#ifdef _MSC_VER
#pragma pack(push, 1)
#define PACKED_STRUCT struct
#define PACKED_END ; _Pragma("pack(pop)")
#else
#define PACKED_STRUCT struct __attribute__((packed))
#endif

#endif

