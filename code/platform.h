#pragma once

#include "stddef.h"
#include "stdint.h"
using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using s8 = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using s64 = int64_t;

using b32 = u32;

using r32 = float;


using uptr = uintptr_t;
using sptr = intptr_t;

#define U32Min (u32(0u))
#define U32Max (~u32(0u))
#define U64Min (u64(0u))
#define U64Max (~u64(0u))

#define Min(A, B) ((A) < (B) ? (A) : (B))
#define Max(A, B) ((A) > (B) ? (A) : (B))
#define ArrayLength(A) (sizeof(A) / sizeof(A[0]))

#ifdef _DEBUG  
#define Assert(Val) if(!(Val)) { *reinterpret_cast<u8 *>(0) = 0; }
#else
#define Assert 
#endif

#define InvalidDefaultCase default: { Assert(!"Invalid default case"); } break;

#define Kilobytes(Value) (Value * 1024)
#define Megabytes(Value) (Kilobytes(Value) * 1024)
#define Gigabytes(Value) (Kilobytes(Value) * 1024)
#define Terabytes(Value) (Kilobytes(Value) * 1024)

