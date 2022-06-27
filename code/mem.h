#pragma once

#define MemoryByteOffset(Begin, End)((u8 *)(End) - (u8 *)(Begin))
void 
MemorySet(void *Dst, uptr Size, u32 Value)
{
  u32 *DstByte = (u32*)(Dst);
  for(uptr ByteIndex = 0;
      ByteIndex < Size / sizeof(u32);
      ++ByteIndex)
  {
    *DstByte++ = Value;
  }
}

void 
MemoryZero(void *Dst, uptr Size)
{
  u8 *DstByte = (u8*)(Dst);
  for(uptr ByteIndex = 0;
      ByteIndex < Size;
      ++ByteIndex)
  {
    *DstByte++ = 0u;
  }
}
#define MemoryZeroStruct(StructPtr)(MemoryZero(StructPtr, sizeof(*StructPtr)))

void * 
MemoryCopy(void *SrcMemory, void *DstMemory, uptr CopySize)
{
  u8 *Src = (u8 *)SrcMemory;
  u8 *Dst = (u8 *)DstMemory;

  for(uptr ByteCount = 0;
      ByteCount < CopySize;
      ++ByteCount)
  {
    *Dst++ = *Src++;
  }
  
  return Dst; 
}
#define MemoryCopyStruct(SrcStruct, DstStruct)(MemoryCopy(SrcStruct, DstStruct, sizeof(*SrcStruct));

void * 
MemoryCopyBackwards(void *SrcMemory, void *DstMemory, uptr CopySize)
{
  u8 *Src = (u8 *)SrcMemory + CopySize - 1;
  u8 *Dst = (u8 *)DstMemory + CopySize - 1;
  for(uptr ByteCount = 0;
      ByteCount < CopySize;
      ++ByteCount)
  {
    *Dst-- = *Src--;
  }
  
  return Dst; 
}

void * 
MemoryCopyReversed(void *SrcMemory, void *DstMemory, uptr CopySize)
{
  u8 *Src = (u8 *)SrcMemory + CopySize - 1;
  u8 *Dst = (u8 *)DstMemory + CopySize - 1;
  for(uptr ByteCount = 0;
      ByteCount < CopySize;
      ++ByteCount)
  {
    *Dst-- = *Src--;
  }
  
  return Dst; 
}

struct memory_pool
{
  void *Memory;
  uptr SizeTotal;
  uptr SizeUsed;
};

uptr
MemoryPoolSizeRemaining(memory_pool *Pool)
{
  uptr Result = Pool->SizeTotal - Pool->SizeUsed;
  return Result;
}

memory_pool 
MemoryPoolCreate(void *Memory, uptr Size)
{
  memory_pool Result;
  Result.Memory = Memory;
  Result.SizeTotal = Size;
  Result.SizeUsed = 0;

  return Result;
}

void
MemoryPoolInit(memory_pool *Pool, void *Memory, uptr Size)
{
  *Pool = MemoryPoolCreate(Memory, Size); 
}

void 
MemoryPoolInit(memory_pool *Pool, memory_pool *SubPool, uptr Size)
{
  Assert(SubPool->SizeUsed + Size <= SubPool->SizeTotal);
  Pool->Memory = (u8 *)SubPool->Memory + SubPool->SizeUsed;
  Pool->SizeTotal = Size;
  Pool->SizeUsed = 0;

  SubPool->SizeUsed += Size;
}
memory_pool 
MemoryPoolCreate(memory_pool *SubPool, uptr Size)
{
  memory_pool Result;
  MemoryPoolInit(&Result, SubPool, Size);

  return Result;
}

memory_pool 
MemoryPoolCreate(memory_pool *SubPool)
{
  memory_pool Result;
  MemoryPoolInit(&Result, SubPool, MemoryPoolSizeRemaining(SubPool));

  return Result;
}

void *
MemoryPoolPushSize(memory_pool *Pool, uptr Size)
{
  Assert(Pool->SizeUsed + Size <= Pool->SizeTotal);
  
  void *Result = ((u8 *)Pool->Memory) + Pool->SizeUsed;
  Pool->SizeUsed += Size;

  return Result;
}

void *
MemoryPoolTryPushSize(memory_pool *Pool, uptr Size)
{
  void *Result = 0;
  if(Pool->SizeUsed + Size <= Pool->SizeTotal)
  {
    Result = MemoryPoolPushSize(Pool, Size);
  }

  return Result;
}

void
MemoryPoolPopSize(memory_pool *Pool, uptr Size)
{
  Assert(Pool->SizeUsed >= Size);
  Pool->SizeUsed -= Size;
}

#define MemoryPoolPushStruct(Pool, StructType)((StructType *)(MemoryPoolPushSize(Pool, sizeof(StructType))))
#define MemoryPoolTryPushStruct(Pool, StructType)((StructType *)(MemoryPoolTryPushSize(Pool, sizeof(StructType))))

#define MemoryPoolPushStructs(Pool, StructType, Count)((StructType *)(MemoryPoolPushSize(Pool, sizeof(StructType) * Count)))
#define MemoryPoolTryPushStructs(Pool, StructType, Count)((StructType *)(MemoryPoolTryPushSize(Pool, sizeof(StructType) * Count)))

#define MemoryPoolPopStruct(Pool, StructType)(MemoryPoolPopSize(Pool, sizeof(StructType)))

void *
MemoryPoolCopyMove(memory_pool *Pool, void *Memory, uptr Size)
{
  void *Result = Pool->Memory;

  MemoryCopy(Pool->Memory, Memory, Pool->SizeUsed);
  Pool->Memory = Memory;
  Pool->SizeTotal = Size;

  return Result;
}

void
MemoryPoolShift(memory_pool *Pool, void *From, void *To)
{
  void *MemoryStart = (u8 *)Pool->Memory;
  void *MemoryEnd = (u8 *)Pool->Memory + Pool->SizeUsed;
  Assert(From >= MemoryStart && From <= MemoryEnd);
  Assert(To >= MemoryStart && To <= MemoryEnd);
  Assert(From >= To);

  uptr MoveSize = (u8 *)MemoryEnd - (u8 *)From;
  MemoryCopy(From, To, MoveSize);

  uptr RemoveSize = (u8 *)From - (u8 *)To;
  Pool->SizeUsed -= RemoveSize;
}

void 
MemoryPoolReset(memory_pool *Pool)
{
  Pool->SizeUsed = 0;
}
// TODO think of better name
struct memory_temporary
{
  memory_pool *Pool;
  uptr RestoreSizeUsed;
};

memory_temporary 
MemoryPoolBeginTemporaryMemory(memory_pool *Pool)
{
 memory_temporary Result;
 Result.Pool = Pool;
 Result.RestoreSizeUsed = Pool->SizeUsed;

 return Result;
}

void
MemoryPoolEndTemporaryMemory(memory_temporary MemoryTemporary)
{
  MemoryTemporary.Pool->SizeUsed = MemoryTemporary.RestoreSizeUsed;
}

void
MemoryPoolResetTemporaryMemory(memory_temporary MemoryTemporary)
{
  MemoryTemporary.Pool->SizeUsed = MemoryTemporary.RestoreSizeUsed;
}

