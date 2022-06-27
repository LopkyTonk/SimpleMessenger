#pragma once
#include "stdarg.h"

struct string
{
  char *Data;
  u32 Length;
};

char 
StringAt(string String, u32 At)
{
  char Result = String.Data[At];
  return Result;
}

string 
StringDataLength(char *Data, u32 Length)
{
  string Result;
  Result.Data = Data;
  Result.Length = Length;

  return Result;
}
#define StringCharArray(CharArray)(StringDataLength(CharArray, sizeof(CharArray) - 1))

string
StringCut(string *Cut, u32 CutLength)
{
  string Result;
  Result.Data = Cut->Data;
  Result.Length = CutLength;

  Cut->Data += CutLength;
  Cut->Length -= CutLength;

  return Result;
}

u32 StringLength(char *Str)
{
  u32 Result = 0;
  for(char *Ch = Str;
      *Ch;
      ++Ch, ++Result);

  return Result;
}

u32 StringEqual(char *Str0, char *Str1)
{
  char *Str0Cursor = Str0;
  char *Str1Cursor = Str1;
  
  // Could be better but heh
  {
    b32 KeepChecking = true;
    while(KeepChecking)
    {
      if(*Str0Cursor == *Str1Cursor)
      {
        ++Str0Cursor;
        ++Str1Cursor;

        KeepChecking = *Str0Cursor && *Str1Cursor;
      }
      else
      {
        KeepChecking = false;
      }
    }
  }

  b32 Result = *Str0Cursor == *Str1Cursor;

  return Result; 
}

u32 StringEqual(string Lhs, string Rhs)
{
  b32 Result = true;
  if(Lhs.Length != Rhs.Length)
  {
    Result = false;
  }
  else
  {
    for(u32 Index = 0;
        Index < Lhs.Length;
        ++Index)
    {
      if(Lhs.Data[Index] != Rhs.Data[Index])
      {
        Result = false;
        break;
      }
    }
  }

  return Result;
}

u32 StringEqual(char *Str0, char *Str1, u32 Size)
{
  char *Str0Cursor = Str0;
  char *Str1Cursor = Str1;
  
  u32 CheckIndex = 0;
  while(true)
  {
    if(CheckIndex == Size)
    {
      break;
    }
    else if(*Str0Cursor++ != *Str1Cursor++)
    {
      break;
    }
    else
    { 
      ++CheckIndex;
    }
  }

  b32 Result = CheckIndex == Size;

  return Result; 
}

char *
StringAppend(char *Src, u32 SrcLength, char *Dst, u32 DstSize)
{
  u32 CopyCount = Min(SrcLength, DstSize);
  for(u32 Index = 0;
      Index < CopyCount;
      ++Index)
  {
    *Dst++ = *Src++;
  }
  
  char *Result = Dst;
  return Result;
}
char *
StringAppend(char *Src, char *Dst, u32 CopyCount)
{
  for(u32 Index = 0;
      CopyCount--;
      ++Index)
  {
    *Dst++ = *Src++;
  }
  char *Result = Dst;

  return Result;
}

u32
StringRemoveSurroundingSpaces(char *Str, u32 StrLength)
{
  u32 Result = 0;
  if(StrLength)
  {
    char *StrEnd = Str + StrLength;
    char *CopyBegin = Str;
    
    u32 FirstLetterIndex = 0;
    while(true)
    {
      if(FirstLetterIndex == StrLength) 
      {
        break;
      }
      else if(Str[FirstLetterIndex] != ' ')
      {
       break;
      }
      else
      {
        ++FirstLetterIndex;
      }
    }
    if(FirstLetterIndex == StrLength)
    {
      Str[0] = '\0';
    }
    else
    {
      u32 LastLetterIndex = StrLength - 1;
      while(true)
      {
        if(Str[LastLetterIndex] != ' ')
        {
          break;
        }
        else
        {
          --LastLetterIndex;
        }
      }
      
      Result = (u32)(LastLetterIndex - FirstLetterIndex + 1);
      if(FirstLetterIndex)
      {
        MemoryCopy(Str + FirstLetterIndex, Str, Result);
      }
      Str[Result] = '\0';
    }

  }

  return Result;
}

b32 
StringHasPrefix(char *Str, char *Prefix)
{
  char *StrCursor = Str;
  char *PrefixCursor = Prefix;

  b32 Result = true;
  while(true)
  {
    if(*PrefixCursor == '\0')
    {
      break;
    }
    else if(*StrCursor != *PrefixCursor)
    { 
      Result = false;
      break;
    }
    else
    {
      ++StrCursor;
      ++PrefixCursor;
    }
  }

  return Result;
}

#define StringInvalidOffset ~0u
u32 
StringNextMatchOffset(char *Str, u32 StrLength, char *Match, u32 MatchLength)
{
  u32 Result = 0;

  while(true)
  {
    if(Result == StrLength)
    {
      break;
    }
    else 
    {
      b32 FoundMatch = false;
      for(u32 MatchIndex = 0;
          MatchIndex < MatchLength;
          ++MatchIndex)
      {
        if(Str[Result] == Match[MatchIndex])
        {
          FoundMatch = true;
          break;
        }
      }

      if(FoundMatch)
      {
        break;
      }
      else
      {
        ++Result;
      }
    }
  }

  return Result;
}
u32
StringNextMatchOffset(string Str, string Match)
{
  u32 Result = StringNextMatchOffset(Str.Data, Str.Length, Match.Data, Match.Length);
  return Result;
}

u32 
StringLastMatchOffset(char *Str, u32 StrLength, char *Match, u32 MatchLength)
{
  u32 Cursor = 0;
  u32 Result = 0;

  while(true)
  {
    if(Cursor == StrLength)
    {
      break;
    }
    else 
    {
      for(u32 MatchIndex = 0;
          MatchIndex < MatchLength;
          ++MatchIndex)
      {
        if(Str[Result] == Match[MatchIndex])
        {
          Result = Cursor;
        }
      }
      
      ++Cursor;
    }
  }

  return Result;
}

u32 
StringNextMatchOffset(char *Str, u32 StrLength, char Match)
{
  b32 Result = StringNextMatchOffset(Str, StrLength, &Match, 1);
  return Result;
}


u32
StringFormatLength(char *Format...)
{
  va_list Args;
  va_start(Args, Format);
  
  u32 Result = 0;
  u32 FormatIndex = 0;
  u32 FormatLength = StringLength(Format);
  
  b32 KeepFormating = true;
  while(KeepFormating)
  {
    if(FormatIndex == FormatLength)
    {
      KeepFormating = false;
    }
    else
    {
      if(Format[FormatIndex] != '(')
      {
        ++Result;
        ++FormatIndex;
      }
      else
      {
        char *Identifier = Format + FormatIndex + 1;
        u32 IdentifierLength = StringNextMatchOffset(Identifier, FormatLength - FormatIndex, ")", 1);
        Assert(IdentifierLength != StringInvalidOffset);
        Assert(IdentifierLength != 0 && "Forgot identifer ending!");

        if(StringEqual(Identifier, "s", 1))
        {
          string String = va_arg(Args, string);
          Result += String.Length;
          FormatIndex += IdentifierLength + 2;
        }
        else
        {
          Assert(!"Unknown type identifier!");
        }

      }
    }
  }
  
  return Result;
}
u32
StringFormat(char *Dst, u32 DstLength, char *Format...)
{
  va_list Args;
  va_start(Args, Format);
  
  u32 DstIndex = 0;
  u32 FormatIndex = 0;
  u32 FormatLength = StringLength(Format);
  
  b32 KeepFormating = true;
  while(KeepFormating)
  {
    if(FormatIndex == FormatLength)
    {
      KeepFormating = false;
    }
    else
    {
      if(Format[FormatIndex] != '(')
      {
        Dst[DstIndex++] = Format[FormatIndex++];
      }
      else
      {
        char *Identifier = Format + FormatIndex + 1;
        u32 IdentifierLength = StringNextMatchOffset(Identifier, FormatLength - FormatIndex, ")", 1);
        Assert(IdentifierLength != StringInvalidOffset);
        Assert(IdentifierLength != 0 && "Forgot identifer ending!");

        if(StringEqual(Identifier, "s", 1))
        {
          string String = va_arg(Args, string);
          StringAppend(String.Data, Dst + DstIndex, String.Length);

          DstIndex += String.Length;
          FormatIndex += IdentifierLength + 2;
        }
        else
        {
          Assert(!"Unknown type identifier!");
        }

      }
    }
  }
  
  DstIndex = Min(DstIndex, DstLength);
  Dst[DstIndex] = '\0';

  u32 Result = DstIndex;
  return Result;
}

