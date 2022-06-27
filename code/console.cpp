struct console_context
{
  HANDLE Out;
  HANDLE In;

  s16 InHeight;
  COORD PrintPos;

  char Title[256];
  u32 TitleLength;

  u32 CharBufferSize;
  CHAR_INFO *CharBuffer;
};
console_context GlobalConCtx;


CONSOLE_SCREEN_BUFFER_INFO
ConsoleGetScreenBufferInfo_()
{
  CONSOLE_SCREEN_BUFFER_INFO Result;
  GetConsoleScreenBufferInfo(GlobalConCtx.Out, &Result); 

  return Result;
}

void
ConsoleSetDefaultPrintCursorPos_(CONSOLE_SCREEN_BUFFER_INFO *ScreenInfo)
{
  GlobalConCtx.PrintPos.X = 0;
  GlobalConCtx.PrintPos.Y = ScreenInfo->dwSize.Y - GlobalConCtx.InHeight - 1;
}


void 
ConsoleSetCursorPos_(COORD NewPos)
{
  SetConsoleCursorPosition(GlobalConCtx.Out, NewPos);
}

#define ConsoleTitleHeight (1)
void
ConsoleSetDefaultReadCursorPos_(CONSOLE_SCREEN_BUFFER_INFO *ScreenInfo)
{
  COORD DefaultPos;
  DefaultPos.X = 0;
  // +1 so tha there is space in between out and in
  DefaultPos.Y = ScreenInfo->dwSize.Y - GlobalConCtx.InHeight + ConsoleTitleHeight;
  ConsoleSetCursorPos_(DefaultPos);
}

u32 
ConsoleInit()
{
  u32 Result = 0;
  b32 AllocResult = AllocConsole() != 0;

  if(!AllocResult)
  {
    Result = GetLastError();
  }
  else
  {
    AttachConsole(ATTACH_PARENT_PROCESS);
    GlobalConCtx.Out = GetStdHandle(STD_OUTPUT_HANDLE);
    GlobalConCtx.In = GetStdHandle(STD_INPUT_HANDLE);

    if(GlobalConCtx.Out == INVALID_HANDLE_VALUE ||
        GlobalConCtx.In == INVALID_HANDLE_VALUE)
    {
      Result = GetLastError();
    }
    else
    {
      GlobalConCtx.InHeight = 2;
      CONSOLE_SCREEN_BUFFER_INFO ScreenInfo = ConsoleGetScreenBufferInfo_();
      

      ConsoleSetDefaultPrintCursorPos_(&ScreenInfo);
      ConsoleSetDefaultReadCursorPos_(&ScreenInfo);
    }
  }

  return Result;
}

void ConsoleClear()
{
  CONSOLE_SCREEN_BUFFER_INFO ScreenInfo = ConsoleGetScreenBufferInfo_();
  u32 ClearCount = ScreenInfo.dwSize.X * (ScreenInfo.dwSize.Y - GlobalConCtx.InHeight);
 DWORD Ignored;
 FillConsoleOutputCharacterA(GlobalConCtx.Out, ' ', ClearCount, COORD{}, &Ignored); 
  ConsoleSetDefaultPrintCursorPos_(&ScreenInfo);
}

void
ConsoleWriteTitle_(CONSOLE_SCREEN_BUFFER_INFO *ScreenInfo)
{
  COORD WritePos;
  WritePos.X = 0;
  WritePos.Y = ScreenInfo->dwSize.Y - GlobalConCtx.InHeight;
  
  {
    DWORD Ignored;
    FillConsoleOutputCharacterA(GlobalConCtx.Out, ' ', ScreenInfo->dwSize.X, WritePos, &Ignored); 
  }
  u32 WriteLength = Min(GlobalConCtx.TitleLength, (u32)(ScreenInfo->dwSize.X));
  ConsoleSetCursorPos_(WritePos); 
  
  DWORD Ignored;
  WriteConsole(GlobalConCtx.Out, GlobalConCtx.Title, WriteLength, &Ignored, 0);
}

void
ConsoleSetTitle(char *NewTitle, u32 NewTitleLength)
{
  GlobalConCtx.TitleLength = Min(NewTitleLength, sizeof(GlobalConCtx.Title) - 1); 
  StringAppend(NewTitle, GlobalConCtx.Title, GlobalConCtx.TitleLength);
  GlobalConCtx.Title[GlobalConCtx.TitleLength] = '\0';

  CONSOLE_SCREEN_BUFFER_INFO ScreenInfo = ConsoleGetScreenBufferInfo_();
  COORD RememberCursor = ScreenInfo.dwCursorPosition;
  ConsoleWriteTitle_(&ScreenInfo);
  ConsoleSetCursorPos_(RememberCursor);
}

  void
ConsoleClearReadScreen_(CONSOLE_SCREEN_BUFFER_INFO *ScreenInfo)
{
  COORD ClearOffset = {};
  ClearOffset.X = 0;
  ClearOffset.Y = ScreenInfo->dwSize.Y  - static_cast<SHORT>(GlobalConCtx.InHeight);
  DWORD ClearCount = ScreenInfo->dwSize.X * static_cast<SHORT>(GlobalConCtx.InHeight);
  DWORD Ignored;
  FillConsoleOutputCharacter(GlobalConCtx.Out, ' ', ClearCount, ClearOffset, &Ignored);
}

  u32 
ConsolePrintA(char *PrintBuffer, u32 PrintCharCount)
{
  //TODO Should put ConsoleInput on hold, but how
  // witout writing more sophisitcated system - processing every input?
  // If that is the only option, then GUI is better alternative since 
  // it's basically the same but better in the end.
  // Perhaps try virtual terminal.
  CONSOLE_SCREEN_BUFFER_INFO ScreenInfo;
  GetConsoleScreenBufferInfo(GlobalConCtx.Out, &ScreenInfo);

  u32 CopyWidth = ScreenInfo.dwMaximumWindowSize.X;
  u32 CopyHeight = GlobalConCtx.InHeight;

  {
    u32 NewCopyBufferSize = CopyWidth * CopyHeight * sizeof(CHAR_INFO);
    if(GlobalConCtx.CharBufferSize < NewCopyBufferSize)
    {
      GlobalConCtx.CharBufferSize = NewCopyBufferSize;

      if(GlobalConCtx.CharBuffer)
      {
        VirtualFree(GlobalConCtx.CharBuffer, 0, MEM_DECOMMIT);
      }
      GlobalConCtx.CharBuffer = reinterpret_cast<CHAR_INFO *>(VirtualAlloc(0, GlobalConCtx.CharBufferSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
    }
  }

  // Copy from the small buffer info before we write
  COORD CopySize = {};
  CopySize.X = static_cast<short>(CopyWidth);
  CopySize.Y = static_cast<short>(CopyHeight);
  COORD CopyOffset = {};
  SMALL_RECT CopyRect = {};
  CopyRect.Left = 0;
  CopyRect.Right = ScreenInfo.dwSize.X;
  CopyRect.Top = ScreenInfo.dwSize.Y  - static_cast<short>(CopyHeight);
  CopyRect.Bottom = ScreenInfo.dwSize.Y;
  DWORD ReadResult = ReadConsoleOutputA(GlobalConCtx.Out, GlobalConCtx.CharBuffer, CopySize, CopyOffset, &CopyRect);

  // Clar input so we dont pollute print!
  ConsoleClearReadScreen_(&ScreenInfo);
  // Remember read position and set for print
  COORD ReadCursorPos = ScreenInfo.dwCursorPosition;
  SetConsoleCursorPosition(GlobalConCtx.Out, GlobalConCtx.PrintPos);

  u32 Result = 0;
  DWORD CharsWritten;
  if(WriteConsoleA(GlobalConCtx.Out, PrintBuffer, PrintCharCount, &CharsWritten, 0) == 0)
  {
    Result = GetLastError();
  }
  // Retrieve new position
  GetConsoleScreenBufferInfo(GlobalConCtx.Out, &ScreenInfo);

  COORD NewPrintPos = ScreenInfo.dwCursorPosition;
  s16 HeightAdvanced = NewPrintPos.Y - GlobalConCtx.PrintPos.Y;
  //if(PrintBuffer[PrintCharCount - 1] == '\n') { --HeightAdvanced; }
  
  if(HeightAdvanced != 0)
  {
    // Scroll
    SMALL_RECT ScrollRect = {};
    ScrollRect.Left = 0;
    ScrollRect.Right = ScreenInfo.dwSize.X;
    ScrollRect.Top = HeightAdvanced;
    ScrollRect.Bottom = ScreenInfo.dwSize.Y;

    COORD ScrollOffset = {};
    //ScrollOffset.Y = 1;
    //ScrollOffset.X = 0;
    //ScrollOffset.Y = -HeightAdvanced + 1;

    SMALL_RECT ClipRect = ScrollRect;
    CHAR_INFO CharFill = {};
    CharFill.Attributes = 0;
    CharFill.Char.AsciiChar = 'B';

    ScrollConsoleScreenBuffer(GlobalConCtx.Out,
        &ScrollRect,
        0,
        ScrollOffset,
        &CharFill);
  }

  ConsoleWriteTitle_(&ScreenInfo);
  ConsoleSetDefaultPrintCursorPos_(&ScreenInfo);
  GlobalConCtx.PrintPos.X = NewPrintPos.X;

  // Write small buffer
  DWORD WriteResult = WriteConsoleOutputA(GlobalConCtx.Out, GlobalConCtx.CharBuffer, CopySize, CopyOffset, &CopyRect);
  // And reset cursor
  SetConsoleCursorPosition(GlobalConCtx.Out, ReadCursorPos);

  return Result;
}

  u32 
ConsolePrintA(char *PrintBuffer)
{
  u32 PrintCharCount = StringLength(PrintBuffer);
  u32 Result = ConsolePrintA(PrintBuffer, PrintCharCount);

  return Result;
}

/*
   u32 
   ConsolePrintW(void *PrintBuffer)
   {
   u32 PrintCharCount = 0;
   for(wchar_t *Char = reinterpret_cast<wchar_t *>(PrintBuffer);
 *Char;
 ++Char, ++PrintCharCount);

 u32 Result = 0;
 DWORD CharsWritten;
 if(WriteConsoleW(GlobalConCtx.Out, PrintBuffer, PrintCharCount, &CharsWritten, 0) == 0)
 {
 Result = GetLastError();
 }

 return Result;
 }
 */

u32
ConsoleReadA(void *DstBuffer, u32 DstBufferSize)
{
  u32 Result = 0;
  DWORD ReadCount;
  if(ReadConsoleA(GlobalConCtx.In, DstBuffer, DstBufferSize - 1, &ReadCount, 0) == 0)
  {
    //Result = GetLastError();
    Result = ~0u;
  }
  else
  { 
    // w/o carriage return and newline character
    if(ReadCount <= 2)
    {
      ReadCount = 0;
    }
    else
    {
      ReadCount -= 2;
      // and clean that
      reinterpret_cast<char *>(DstBuffer)[ReadCount] = '\0' ;

      Result = ReadCount;
    }
  }

  CONSOLE_SCREEN_BUFFER_INFO ScreenInfo;
  GetConsoleScreenBufferInfo(GlobalConCtx.Out, &ScreenInfo);
  
  // shitty fix but what shoud i say
  // enter moves the whole thing up
  {
    // Scroll
    SMALL_RECT ScrollRect = {};
    ScrollRect.Left = 0;
    ScrollRect.Right = ScreenInfo.dwSize.X;
    ScrollRect.Top = 0;
    ScrollRect.Bottom = ScreenInfo.dwSize.Y;

    COORD ScrollOffset = {};
    ScrollOffset.Y = 1;
    
    CHAR_INFO CharFill = {};
    CharFill.Attributes = 0;
    CharFill.Char.AsciiChar = ' ';

    ScrollConsoleScreenBuffer(GlobalConCtx.Out,
        &ScrollRect,
        0,
        ScrollOffset,
        &CharFill);
  }

  ConsoleClearReadScreen_(&ScreenInfo);
  ConsoleWriteTitle_(&ScreenInfo);
  ConsoleSetDefaultReadCursorPos_(&ScreenInfo);

  return Result;
}

u32
ConsoleReadEmpty()
{
  char CharBuffer[256];
  u32 Result = ConsoleReadA(CharBuffer, sizeof(CharBuffer));

  return Result;
}
