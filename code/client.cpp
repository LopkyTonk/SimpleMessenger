#include "platform.h"

#include "intrinsics.h"
#include "mem.h"
#include "mat.h"
#include "str.h"

#include "win32.h"
#include "resources.h"
#include "file.h"
#include "render.h"
#include "ui.h"

#include "requests_and_responses.h"

struct win32_window
{
  u32 WidthMax;
  u32 HeightMax;
  bitmap Bitmap;

  HWND Handle;
  u32 Quit;

  BITMAPINFO BitmapInfo;

  s16 ScrollOffset;
};

u32 AtomicExchange(volatile u32* Value, u32 NewValue)
{
  u32 Result = InterlockedExchange((volatile LONG*)(Value), NewValue);

  return Result;
}


// TODO better place
win32_window *WindowNative;
ui_window *WindowMain;

  LRESULT 
Win32WindowProc(HWND Window, UINT Msg, WPARAM WParam, LPARAM LParam)
{
  LRESULT Result = 0;
  switch(Msg)
  {
    case WM_LBUTTONDOWN:
      {
        WindowMain->MouseDown = true;
      } break;
      
    case WM_MOUSEWHEEL:
    {
      s16 RawScrollOffset = (s16)(WParam >> 16) & 0xffff; 
      s32 ScrollOffset = -(s32)RawScrollOffset / 120 * 10;
      
      WindowMain->ScrollOffset = ScrollOffset;
    } break;
    case WM_MOUSEMOVE:
    {
      POINT CursorPos;
      CursorPos.x = (LONG)(LParam >> 0) & 0xffff;
      CursorPos.y = (LONG)(LParam >> 16) & 0xffff;

      WindowMain->CursorPosition = V2S32(CursorPos.x, CursorPos.y);
    } break;
    case WM_GETMINMAXINFO:
      {
        MINMAXINFO *MinMaxInfo = (MINMAXINFO *)(LParam);
        MinMaxInfo->ptMaxSize.x = (long)WindowNative->WidthMax;
        MinMaxInfo->ptMaxSize.y = (long)WindowNative->HeightMax;
      } break;
    case WM_SIZE:
      {
        s16 NewWidth = (s16)LOWORD(LParam);
        s16 NewHeight = (s16)HIWORD(LParam);

        WindowNative->Bitmap = Bitmap(WindowNative->Bitmap.Pixels, NewWidth, NewHeight);
        WindowMain->Bitmap = BitmapView(&WindowMain->Bitmap_, NewWidth, NewHeight, 0, 0);
      } break;
    case WM_CLOSE:
    {
      DestroyWindow(Window);
    } break;
    case WM_DESTROY:
    {
      PostQuitMessage(0);
      WindowNative->Quit = true;

    } break;
    case WM_CHAR:
    {
      u32 Keycode = (u32)(WParam);
      ui_widget *ActiveWidget = WindowMain->WidgetWithState[UIWidgetState_Active];
      if(ActiveWidget)
      {
        switch(ActiveWidget->Type)
        {
          case UIWidgetType_InputBox:
            {
              if(Keycode >= ' ' && Keycode <= '~')
              {
                UIPushKeycode(UIGetInputBox(ActiveWidget), (s8)Keycode);
              }
            } break;
          case UIWidgetType_ButtonBox:
          case UIWidgetType_TextBox:
            {
            } break;
            InvalidDefaultCase;
        }
      }

    };
    default:
    {
      Result = DefWindowProc(Window, Msg, WParam, LParam);
    } break;
  }

  return Result;
}

/*
 * Menu location
 * - Start
 * -- Login
 * -- CreateAccount
 * - Logged
 * -- Messaging
 * -- Room create
 */


struct tab_login
{
  ui_input_box *UsernameInput;
  ui_input_box *PasswordInput;
  ui_button_box *LoginButton;
  ui_text_box *Error;
};
struct tab_create_account
{
  ui_input_box *UsernameInput;
  ui_input_box *PasswordInput;
  ui_input_box *PasswordInputRepeat;

  ui_button_box *CreateAccountButton;
  ui_text_box *Error;
  char *ErrorBuffer;
  u32 ErrorBufferSize;
};

enum tab_state
{
  TabState_NeedToInitialize,
  TabState_Running,
  TabState_NeedToFree,
};

enum start_tab
{
  StartTab_Login,
  StartTab_CreateAccount,

  StartTab_Count,
  StartTab_First = 0,
  StartTab_OnePastLast = StartTab_Count
};
struct location_start
{
  ui_form *TopForm;
  ui_form *MainForm;
  
  ui_text_box *TabBoxes[StartTab_Count];

  memory_pool TabTransientPool;
  start_tab Tab;
  tab_state TabState;
  union
  {
    tab_login TabLogin;
    tab_create_account TabCreateAccount;
  };
};

// -- Messaging 
struct tab_messaging
{
  ui_form *SideForm;
  ui_form *MainForm;
  ui_input_box *InputBox;
  ui_button_box *DestroyRoomButton;
};

// -- Room create
struct tab_room_create
{
  ui_form *MainForm;
  ui_input_box *RoomnameInput;
  ui_button_box *CreateButton;

  char *ErrorBuffer;
  u32 ErrorBufferSize;
  ui_text_box *Error;
};

enum logged_tab
{
  LoggedTab_Messaging,
  LoggedTab_CreateRoom,
  LoggedTab_Exit,
  LoggedTab_Count,

  LoggedTab_First = 0,
  LoggedTab_Last = LoggedTab_Count - 1
};

// - Logged
struct location_logged
{
  ui_form *TopForm;
  ui_text_box *TabBoxes[LoggedTab_Count];
  
  memory_pool RoomPool;
  u32 RoomCount;
  struct room *CurrentRoom;


  struct client_info *FirstClientInfo;
  uptr ClientBufferSizeMax;
  uptr ClientBufferSize;
  u32 ClientInfoCount;

  memory_pool TabTransientPool;
  logged_tab Tab;
  tab_state TabState;

  union
  {
    tab_messaging TabMessaging;
    tab_room_create TabCreateRoom;
  };
};

struct message
{
  message *Next;

  u32 ID;
  u32 Length;
};

char *
MessageMessage(message *Message)
{
  char *Result = (char *)(Message + 1);
  return Result;
}

message *
MessageNext(message *Message)
{
  uptr ByteOffset = sizeof(message) + Message->Length;
  message *Result = (message *)((u8 *)Message + ByteOffset);
  return Result;
}

struct room
{
  u32 ID;
  u32 NameLength;

  ui_room *Widget;
  
  memory_pool MessagePool;
  message *FirstMessageInactive;
  u32 MessageCount;
  u32 MessageActiveCount;
};

char *
RoomName(room *Room)
{
  char *Result =(char *)((u8 *)Room + sizeof(room));
  return Result;
}
room *
RoomNext(room *Room)
{
  room *Result = (room *)((u8 *)Room + sizeof(room) + Room->NameLength);
  return Result;
}


enum menu_location
{
  MenuLocation_Start,
  MenuLocation_Logged,
};

// - Login
struct menu_context
{
  memory_pool Pool;
  memory_temporary PoolReset;

  menu_location Location;
  b32 LocationInitialized;
  union
  {
    location_start LocationStart;
    location_logged LocationLogged;
  };
};

void
SetLocation(menu_context *MenuContext, menu_location NewLocation)
{
  MenuContext->Location = NewLocation;
  MenuContext->LocationInitialized = false;
};

void
RoomRemove(room *Remove, location_logged *LocationLogged)
{
}
enum request_stage
{
  RequestStage_Idle,
  RequestStage_Wait,
  RequestStage_TimedOut,
  RequestStage_Ready,
  RequestStage_Cooldown,
};

struct request_state
{
  request_stage Stage;
  // in Ms
  u64 Timeout;
  void *Data;
};
#define TimeoutInfinite (~u64(0))
#include "stdio.h"

int WinMain(HINSTANCE Instance,
    HINSTANCE PrevInstante,
    LPSTR CmdLine, 
    int ShowCmd)

{
  /*struct file_font_header
  {
    u64 GlyphOffset; //file_glyph[MaxCodepoint]
    u64 CodepointGlyphMapOffset; //u32[MaxCodepoint]
    u64 KerningsOffset; // r32[MaxCodepoint * MaxCodepoint]
  };


struct resource_font
{
  font_glyph *Glyphs;
  
  u32 *CodepointGlyphMap;
  // 2D array MaxCodepoint * MaxCodepoint
  r32 *Kernings;
};
*/
  resource_font Font;
{
  HANDLE FontFileHandle = CreateFileA("font_file.ff", GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, 0);
  if(FontFileHandle == INVALID_HANDLE_VALUE)
  {
    return -1;
  }

  auto ReadFileData = [&FontFileHandle](u64 Offset, void *Dst, u32 DstSize)
  {
    LONG OffsetLow = (LONG)Offset;
    LONG OffsetHigh = (LONG)(Offset >> 32);

    SetFilePointer(FontFileHandle, OffsetLow, &OffsetHigh, 0);

    DWORD Ignored;
    ReadFile(FontFileHandle, Dst, DstSize, &Ignored, 0);
  };

  file_font_header FontHeader;
  ReadFileData(0, &FontHeader, sizeof(FontHeader));
  if(FontHeader.MagicValue != FontHeaderMagicValue)
  {
    return -1;
  }
  Font.LineHeight = FontHeader.LineHeight;
  Font.GlyphCount = FontHeader.GlyphCount;
  Font.MaxCodepoint = FontHeader.MaxCodepoint;

  Font.Glyphs = (font_glyph*)(Win32Alloc(sizeof(font_glyph) * FontHeader.GlyphCount));
  u64 FileGlyphOffset = FontHeader.GlyphOffset + sizeof(file_glyph);

  u32 *BitmapMemory = (u32 *)Win32Alloc(FontHeader.BitmapSize);
  ReadFileData(FontHeader.BitmapOffset, BitmapMemory, (u32)FontHeader.BitmapSize);
  for(u32 GlyphIndex = 1;
      GlyphIndex < FontHeader.GlyphCount;
      ++GlyphIndex)
  {
    font_glyph *Glyph = Font.Glyphs + GlyphIndex;
    file_glyph FileGlyph;
    ReadFileData(FileGlyphOffset, &FileGlyph, sizeof(FileGlyph));

    Glyph->Codepoint = FileGlyph.Codepoint;
    Glyph->VerticalAdvance = FileGlyph.VerticalAdvance;

    resource_bitmap *ResourceBitmap = &Glyph->ResourceBitmap;
    ResourceBitmap->Align = V2(FileGlyph.BitmapAlignX, FileGlyph.BitmapAlignY); 
    ResourceBitmap->Bitmap = Bitmap(BitmapMemory, FileGlyph.BitmapWidth, FileGlyph.BitmapHeight);

    FileGlyphOffset += sizeof(file_glyph);
    BitmapMemory += FileGlyph.BitmapWidth * FileGlyph.BitmapHeight;
  }

  Font.CodepointGlyphMap = (u32 *)Win32Alloc(FontHeader.CodepointGlyphMapSize);
  ReadFileData(FontHeader.CodepointGlyphMapOffset, Font.CodepointGlyphMap, (u32)FontHeader.CodepointGlyphMapSize);

  Font.Kernings = (r32 *)Win32Alloc(FontHeader.KerningsSize);
  ReadFileData(FontHeader.KerningsOffset, Font.Kernings, (u32)FontHeader.KerningsSize);
}


  void *MessageBuffer = Win32Alloc(2 * Message_SizeMax);
  char *SendBuffer = (char *)(MessageBuffer);
  char *RecvBuffer = (char *)(MessageBuffer) + Message_SizeMax;
  
  u32 RecvSize = 0;

  message_header *SendMessageHeader = (message_header *)(SendBuffer);
  message_header *RecvMessageHeader = (message_header *)(RecvBuffer);
  
  SOCKET Server = INVALID_SOCKET;
  auto SendMessage = [&SendMessageHeader, &Server](rr_type RRType, uptr RequestSize)
  {
    SendMessageHeader->RRType = RRType;

    WSABUF Buffer;
    Buffer.len = (ULONG)(sizeof(message_header) + RequestSize);
    Buffer.buf = (char *)SendMessageHeader;

    DWORD Ignored;
    s32 SendResult = WSASend(Server, &Buffer, 1, &Ignored, 0, 0, 0);
  };

  SendMessageHeader->MagicValue = MessageHeaderMagicValue;

  // setup windows
  win32_window WindowNative_ = {};
  WindowNative = &WindowNative_;
  WindowNative->WidthMax = 1920;
  WindowNative->HeightMax = 1080;
  u32 BitmapSize = sizeof(u32) * WindowNative->WidthMax * WindowNative->HeightMax;
  WindowNative->Bitmap = Bitmap((u32 *)Win32Alloc(BitmapSize), 0, 0);

  u32 WindowMemorySize = Megabytes(50);
  void *Memory = VirtualAlloc((void *)Terabytes(2), WindowMemorySize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
  ui_window WindowMain_ = UIWindow(Memory, WindowMemorySize, 1920, 1080, 0xff000099);
  WindowMain = &WindowMain_;

  char ClassName[] = "WINDOW_SERVER";
  WNDCLASS WC = {};
  WC.lpfnWndProc = Win32WindowProc;
  WC.hInstance = Instance;
  WC.lpszClassName = ClassName;
  WC.hCursor = LoadCursor(NULL, IDC_ARROW); 

  RegisterClass(&WC);

  WindowNative->Handle = CreateWindowEx(0, ClassName, "client", WS_OVERLAPPEDWINDOW, 
      CW_USEDEFAULT,CW_USEDEFAULT,
      CW_USEDEFAULT,CW_USEDEFAULT,
      0, 0, Instance, 0);
  ShowWindow(WindowNative->Handle, SW_SHOW);

  int Result = 0;
  if(!WindowNative->Handle)
  {
    Result = -1;
  }
  else
  {
    WindowNative->Quit = false;

    WORD WSAVersion = MAKEWORD(2, 2); 
    WSADATA WSAData;
    DWORD StartupResult = WSAStartup(WSAVersion, &WSAData);

    if(StartupResult != 0)
    {
      //ConsolePrintA("[CLIENT]: Failed to startup WSA!\n");
      Result = StartupResult;
    }
    else
    {
      ADDRINFOA Hints = {};
      Hints.ai_family = AF_UNSPEC;
      Hints.ai_socktype = SOCK_STREAM;
      Hints.ai_protocol = IPPROTO_TCP;

      char *ServerAddress = "localhost";
      ADDRINFOA *ServerAddressInfo;
      u32 GetAddrResult = getaddrinfo(ServerAddress, "12121", &Hints, &ServerAddressInfo);
      if(GetAddrResult)
      {
        Result = GetAddrResult;
      }
      else
      {
        LARGE_INTEGER Frequency, LastTime;
        QueryPerformanceFrequency(&Frequency);
        QueryPerformanceCounter(&LastTime);
        u64 DeltaMs = 0;
        

        request_state RequestStates[RRType_Count] = {};

        enum connection_status
        {
          ConnectionStatus_Disconnected,
          ConnectionStatus_Connected
        };
        connection_status ConnectionStatus = ConnectionStatus_Disconnected;

        menu_context MenuContext = {};
        u32 MemorySize = Megabytes(10);
        MenuContext.Pool = MemoryPoolCreate(Win32Alloc(MemorySize), MemorySize); 
        MenuContext.PoolReset = MemoryPoolBeginTemporaryMemory(&MenuContext.Pool);

        // main loop
        while(!WindowNative->Quit)
        {
          // create_account screen
          switch(MenuContext.Location)
          {
            case MenuLocation_Start:
              {
                location_start *LocationStart = &MenuContext.LocationStart;
                if(!MenuContext.LocationInitialized)
                {
                  MenuContext.LocationInitialized = true;

                  MemoryPoolResetTemporaryMemory(MenuContext.PoolReset);
                  MemoryZeroStruct(LocationStart);

                  LocationStart->TopForm = UIPushForm(WindowMain, UIFormLayout_LeftToRight, V2(0.0f, 0.0f), V2(1.0f, 0.125f));
                  {
                    auto PushTab = [&](string Text) -> ui_text_box *
                    {
                      ui_text_box *Result = UIPushTextBox(LocationStart->TopForm);
                      Result->Text = Text;
                      Result->TextColor = 0xffffffff;
                      UISetFillColor(Result, 0xff007fff);
                      UISetOutline(Result, 1.0f, 0xff000000);

                      return Result;
                    };

                    LocationStart->TabBoxes[StartTab_Login] = PushTab(StringCharArray("Login"));
                    LocationStart->TabBoxes[StartTab_CreateAccount] = PushTab(StringCharArray("CreateAccount"));
                  }
                  LocationStart->MainForm = UIPushForm(WindowMain, UIFormLayout_TopToBottom, V2(0.0f, 0.125f), V2(1.0f, 0.875f));

                  LocationStart->TabTransientPool = MemoryPoolCreate(&MenuContext.Pool, Megabytes(5));
                }

                start_tab TabNext = {};
                for(start_tab CheckTabType = StartTab_First;
                    CheckTabType < StartTab_OnePastLast;
                    CheckTabType = (start_tab)(CheckTabType + 1))
                {
                  if(CheckTabType != LocationStart->Tab)
                  {
                    if(UIIsState(LocationStart->TabBoxes[CheckTabType], UIWidgetState_Active))
                    {
                      TabNext = CheckTabType;
                      LocationStart->TabState = TabState_NeedToFree;
                      break;
                    }
                  }
                }
                
                switch(LocationStart->Tab)
                {
                  case StartTab_Login:
                  {
                    tab_login *TabLogin = &LocationStart->TabLogin;
                    switch(LocationStart->TabState)
                    {
                      case TabState_NeedToInitialize:
                      {
                        MemoryZeroStruct(TabLogin);

                        ui_text_box *UsernameText = UIPushTextBox(LocationStart->MainForm);
                        UsernameText->Text = StringCharArray("Username:");
                        UsernameText->TextColor = 0xffffffff;

                        TabLogin->UsernameInput = UIPushInputBox(LocationStart->MainForm);
                        TabLogin->UsernameInput->Filter = UIInputFilter_FirstNumberOrLetter;
                        UISetWidthMin(TabLogin->UsernameInput, UISizeType_Relative, 0.8f);

                        ui_text_box *PasswordText = UIPushTextBox(LocationStart->MainForm);
                        PasswordText->Text = StringCharArray("Password:");
                        PasswordText->TextColor = 0xffffffff;

                        TabLogin->PasswordInput = UIPushInputBox(LocationStart->MainForm);
                        UISetWidthMin(TabLogin->PasswordInput, UISizeType_Relative, 0.8f);

                        TabLogin->LoginButton = UIPushButtonBox(LocationStart->MainForm);
                        TabLogin->LoginButton->Text = StringCharArray("Login");
                        TabLogin->LoginButton->TextColor = 0xff000000;
                        UISetFillColor(TabLogin->LoginButton, 0xffffffff);

                        TabLogin->Error = UIPushTextBox(LocationStart->MainForm);

                        LocationStart->TabState = TabState_Running;
                      } break;
                      case TabState_Running:
                      {
                        request_state *LoginState = RequestStates + RRType_Login;
                        switch(LoginState->Stage)
                        {
                          case RequestStage_Idle:
                            {
                              if(UIIsEvent(TabLogin->LoginButton, UIWidgetEvent_Pressed))
                              {
                                if(ConnectionStatus == ConnectionStatus_Disconnected)
                                {
                                  TabLogin->Error->Text = StringCharArray("Unable to connect to server...");
                                  TabLogin->Error->TextColor = 0xff0000ff;
                                }
                                else
                                {
                                  string Username = TabLogin->UsernameInput->CharBuffer->String;
                                  string Password = TabLogin->PasswordInput->CharBuffer->String;

                                  if(Username.Length == 0)
                                  {
                                    TabLogin->Error->Text = StringCharArray("Username is empty");
                                    TabLogin->Error->TextColor = 0xff0000ff;
                                  }
                                  else if(Password.Length == 0)
                                  {
                                    TabLogin->Error->Text = StringCharArray("Password is empty");
                                    TabLogin->Error->TextColor = 0xff0000ff;
                                  }
                                  else
                                  {
                                    socket_poll_result WriteResult = SocketPollWrite(Server);
                                    if(!WriteResult.State)
                                    {
                                      TabLogin->Error->Text = StringCharArray("Pressed button at the wrong time lol...");
                                      TabLogin->Error->TextColor = 0xff0000ff;
                                    }
                                    else
                                    {
                                      request_login *Login = MessageHeaderRequestLogin(SendMessageHeader);

                                      Login->UsernameLength = Username.Length;
                                      Login->PasswordLength = Password.Length;

                                      MemoryCopy(Username.Data, RequestLoginUsername(Login), Username.Length);
                                      MemoryCopy(Password.Data, RequestLoginPassword(Login), Password.Length);

                                      uptr RequestSize = sizeof(request_login) + Username.Length + Password.Length;
                                      SendMessage(RRType_Login, RequestSize);

                                      TabLogin->Error->Text = StringCharArray("Awaiting server...");
                                      TabLogin->Error->TextColor = 0xff00a5ff;

                                      LoginState->Stage = RequestStage_Wait;
                                      LoginState->Timeout = 3000;
                                    }
                                  }
                                }
                              }
                            } break;
                          case RequestStage_TimedOut:
                          {
                            TabLogin->Error->Text = StringCharArray("Server did not respond, try again...");
                            TabLogin->Error->TextColor = 0xff00a5ff;
                          };
                          case RequestStage_Ready:
                            {
                              response_login *Login = MessageHeaderResponseLogin(RecvMessageHeader);
                              switch(Login->Error)
                              {
                                case ResponseLoginError_None:
                                  {
                                    UIPopAll(WindowMain);
                                    SetLocation(&MenuContext, MenuLocation_Logged);
                                  } break;
                                case ResponseLoginError_InvalidUsernameOrPassword:
                                  {
                                    TabLogin->Error->Text = StringCharArray("Error: Invalid username or password, try again");
                                    TabLogin->Error->TextColor = 0xff0000ff;
                                  } break;
                              }
                            } break;
                        }
                      } break;
                      case TabState_NeedToFree:
                      {
                      } break;
                    } 
                  } break;
                  case StartTab_CreateAccount:
                  {
                    tab_create_account *TabCreateAccount = &LocationStart->TabCreateAccount;
                    switch(LocationStart->TabState)
                    {
                      case TabState_NeedToInitialize:
                        {
                          MemoryZeroStruct(TabCreateAccount);

                          ui_text_box *UsernameText = UIPushTextBox(LocationStart->MainForm);
                          UsernameText->Text = StringCharArray("Username:");
                          UsernameText->TextColor = 0xffffffff;

                          TabCreateAccount->UsernameInput = UIPushInputBox(LocationStart->MainForm);
                          TabCreateAccount->UsernameInput->Filter = UIInputFilter_FirstNumberOrLetter;
                          UISetWidthMin(TabCreateAccount->UsernameInput, UISizeType_Relative, 0.8f);

                          ui_text_box *PasswordText = UIPushTextBox(LocationStart->MainForm);
                          PasswordText->Text = StringCharArray("Password:");
                          PasswordText->TextColor = 0xffffffff;

                          TabCreateAccount->PasswordInput = UIPushInputBox(LocationStart->MainForm);
                          UISetWidthMin(TabCreateAccount->PasswordInput, UISizeType_Relative, 0.8f);

                          TabCreateAccount->CreateAccountButton = UIPushButtonBox(LocationStart->MainForm);
                          TabCreateAccount->CreateAccountButton->Text = StringCharArray("CreateAccount");
                          TabCreateAccount->CreateAccountButton->TextColor = 0xff000000;
                          UISetFillColor(TabCreateAccount->CreateAccountButton, 0xffffffff);

                          TabCreateAccount->Error = UIPushTextBox(LocationStart->MainForm);
                          
                          TabCreateAccount->ErrorBufferSize = 4096;
                          TabCreateAccount->ErrorBuffer = (char *)MemoryPoolPushSize(&LocationStart->TabTransientPool, TabCreateAccount->ErrorBufferSize);

                          LocationStart->TabState = TabState_Running;
                        } break;

                      case TabState_Running:
                        {
                          request_state *CreateAccountState = RequestStates + RRType_CreateAccount;
                          switch(CreateAccountState->Stage)
                          {
                            case RequestStage_Idle:
                              {
                                if(UIIsEvent(TabCreateAccount->CreateAccountButton, UIWidgetEvent_Pressed))
                                {
                                  if(ConnectionStatus == ConnectionStatus_Disconnected)
                                  {
                                    TabCreateAccount->Error->Text = StringCharArray("Unable to connect to server...");
                                    TabCreateAccount->Error->TextColor = 0xff0000ff;
                                  }
                                  else
                                  {
                                    string Username = TabCreateAccount->UsernameInput->CharBuffer->String;
                                    string Password = TabCreateAccount->PasswordInput->CharBuffer->String;

                                    if(Username.Length == 0)
                                    {
                                      TabCreateAccount->Error->Text = StringCharArray("Username is empty");
                                      TabCreateAccount->Error->TextColor = 0xff0000ff;
                                    }
                                    else if(Password.Length == 0)
                                    {
                                      TabCreateAccount->Error->Text = StringCharArray("Password is empty");
                                      TabCreateAccount->Error->TextColor = 0xff0000ff;
                                    }
                                    else
                                    {
                                      socket_poll_result WriteResult = SocketPollWrite(Server);
                                      if(!WriteResult.State)
                                      {
                                        TabCreateAccount->Error->Text = StringCharArray("Pressed button at the wrong time lol...");
                                        TabCreateAccount->Error->TextColor = 0xff0000ff;
                                      }
                                      else
                                      {
                                        request_create_account *RequestCreateAccount = MessageHeaderRequestCreateAccount(SendMessageHeader);

                                        RequestCreateAccount->UsernameLength = Username.Length;
                                        RequestCreateAccount->PasswordLength = Password.Length;

                                        MemoryCopy(Username.Data, RequestCreateAccountUsername(RequestCreateAccount), Username.Length);
                                        MemoryCopy(Password.Data, RequestCreateAccountPassword(RequestCreateAccount), Password.Length);

                                        uptr RequestSize = sizeof(request_create_account) + Username.Length + Password.Length;
                                        SendMessage(RRType_CreateAccount, RequestSize);

                                        TabCreateAccount->Error->Text = StringCharArray("Awaiting server...");
                                        TabCreateAccount->Error->TextColor = 0xff00a5ff;

                                        CreateAccountState->Stage = RequestStage_Wait;
                                        CreateAccountState->Timeout = TimeoutInfinite;
                                      }
                                    }
                                  }
                                }
                              } break;
                            case RequestStage_Ready:
                              {
                                response_create_account *ResponseCreateAccount = MessageHeaderResponseCreateAccount(RecvMessageHeader);

                                string *Username = &TabCreateAccount->UsernameInput->CharBuffer->String;
                                string *Password = &TabCreateAccount->PasswordInput->CharBuffer->String;
                                switch(ResponseCreateAccount->Error)
                                {
                                  case ResponseCreateAccountError_None:
                                    {
                                      u32 ErrorBufferLength = StringFormat(TabCreateAccount->ErrorBuffer, TabCreateAccount->ErrorBufferSize,
                                          "Success: Account with name \"(s)\" was created successfully!", *Username);

                                      TabCreateAccount->Error->Text = StringDataLength((char *)TabCreateAccount->ErrorBuffer, ErrorBufferLength);
                                      TabCreateAccount->Error->TextColor = 0xff00ff00;
                                    } break;
                                  case ResponseCreateAccountError_UsernameExists:
                                    {
                                      u32 ErrorBufferLength = StringFormat(TabCreateAccount->ErrorBuffer, TabCreateAccount->ErrorBufferSize,
                                          "Error: Account with name \"(s)\" already exists!", *Username);
                                      TabCreateAccount->Error->Text = StringDataLength((char *)TabCreateAccount->ErrorBuffer, ErrorBufferLength);
                                      TabCreateAccount->Error->TextColor = 0xff0000ff;
                                    } break;
                                  case ResponseCreateAccountError_TooManyAccounts:
                                    {
                                      TabCreateAccount->Error->Text = StringCharArray("Error: Too many accounts");
                                      TabCreateAccount->Error->TextColor = 0xff0000ff;
                                    } break;
                                }

                                Username->Length = 0;
                                Password->Length = 0;
                              } break;
                          }
                        } break;
                      case TabState_NeedToFree:
                        {
                        } break;
                    }
                  } break;
                }
                if(LocationStart->TabState == TabState_NeedToFree)
                {
                  UIPopAll(LocationStart->MainForm);
                  LocationStart->TabState = TabState_NeedToInitialize;
                  LocationStart->Tab = TabNext;
                  MemoryPoolReset(&LocationStart->TabTransientPool);
                }
                {
#if 0
#endif 
                }
              } break;
            case MenuLocation_Logged:
              {
                if(ConnectionStatus == ConnectionStatus_Disconnected)
                {
                  UIPopAll(WindowMain);
                  SetLocation(&MenuContext, MenuLocation_Start);
                }
                else
                {
                  location_logged *LocationLogged = &MenuContext.LocationLogged;
                  if(!MenuContext.LocationInitialized)
                  {
                    MenuContext.LocationInitialized = true;

                    MemoryPoolResetTemporaryMemory(MenuContext.PoolReset);
                    MemoryZeroStruct(LocationLogged);
                    
                    LocationLogged->TabTransientPool = MemoryPoolCreate(&MenuContext.Pool, Megabytes(5));
                    LocationLogged->ClientBufferSizeMax = Megabytes(1);
                    LocationLogged->FirstClientInfo = (client_info *)MemoryPoolPushSize(&MenuContext.Pool, LocationLogged->ClientBufferSizeMax);

                    LocationLogged->RoomPool = MemoryPoolCreate(Win32Alloc(Megabytes(1)), Megabytes(1));
                    
                    LocationLogged->TopForm = UIPushForm(WindowMain, UIFormLayout_LeftToRight, V2(0.0f, 0.0f), V2(1.0f, 0.1f));
                    {
                      UIFormSetBackgroundColor(LocationLogged->TopForm, 0xff00a5ff);

                      auto PushTab = [&](string Text) -> ui_text_box *
                      {
                        ui_text_box *Result = UIPushTextBox(LocationLogged->TopForm);
                        Result->Text = Text;
                        Result->TextColor = 0xffffffff;
                        UISetFillColor(Result, 0xff007fff);
                        UISetOutline(Result, 1.0f, 0xff000000);

                        return Result;
                      };
                      LocationLogged->TabBoxes[LoggedTab_Messaging] = PushTab(StringCharArray("Messaging"));
                      LocationLogged->TabBoxes[LoggedTab_CreateRoom] = PushTab(StringCharArray("Create room"));
                      LocationLogged->TabBoxes[LoggedTab_Exit] = PushTab(StringCharArray("Exit"));
                    }
                    
                  }
                  
                  {
                    request_state *RoomListState = RequestStates + RRType_RoomList;
                    switch(RoomListState->Stage)
                    {
                      case RequestStage_Idle:
                        {
                          socket_poll_result WriteResult = SocketPollWrite(Server);
                          if(WriteResult.Events & POLLWRNORM)
                          {
                            SendMessage(RRType_RoomList, sizeof(request_room_list));
                            RoomListState->Stage = RequestStage_Wait;
                            RoomListState->Timeout = 1000;
                          }
                        } break;

                      case RequestStage_Ready:
                      {
                        response_room_list *RoomList = MessageHeaderResponseRoomList(RecvMessageHeader);
                        
                        b32 RoomRemoved = false;
                        room *FindRoom = (room *)LocationLogged->RoomPool.Memory;
                        for(u32 RoomIndex = 0 ;
                            RoomIndex < LocationLogged->RoomCount;)
                        {
                          b32 KeepRoom = false;

                          room_info *RoomInfo = RoomInfoFirst(RoomList);
                          for(u32 RoomInfoIndex = 0 ;
                              RoomInfoIndex < RoomList->RoomInfoCount;
                              ++RoomInfoIndex)
                          {
                            if(RoomInfo->ID == FindRoom->ID)
                            {
                              KeepRoom = true;
                              break;
                            }
                            else
                            {
                              RoomInfo = RoomInfoNext(RoomInfo);
                            }
                          }

                          if(KeepRoom)
                          {
                            u8 *CopyBegin = (u8 *)RoomInfoNext(RoomInfo);
                            u8 *CopyEnd = (u8 *)RecvBuffer + RecvSize;
                            u8 *CopyDst = (u8 *)(RoomInfo);
                            MemoryCopy(CopyBegin, CopyDst, CopyEnd - CopyBegin);

                            --RoomList->RoomInfoCount;
                            ++RoomIndex;
                            FindRoom = RoomNext(FindRoom);
                          }
                          else
                          {
                            RoomRemoved = true;
                            if(LocationLogged->CurrentRoom == FindRoom)
                            {
                              UIPopAllOfType(WindowMain, UIWidgetType_Message);
                              if(LocationLogged->RoomCount >= 2)
                              {
                                LocationLogged->CurrentRoom = (room *)LocationLogged->RoomPool.Memory;
                              }
                              else
                              {
                                LocationLogged->CurrentRoom = 0;
                              }
                            }
                            Win32Free(FindRoom->MessagePool.Memory);
                            MemoryPoolShift(&LocationLogged->RoomPool, RoomNext(FindRoom), FindRoom);

                            --LocationLogged->RoomCount;

                            RoomInfo = RoomInfoNext(RoomInfo);
                          }
                        }
                        if(RoomRemoved)
                        {
                          UIPopAllOfType(WindowMain, UIWidgetType_Room);
                          room *ClearRoom = (room *)LocationLogged->RoomPool.Memory;
                          for(u32 RoomIndex = 0;
                              RoomIndex < LocationLogged->RoomCount;
                              ++RoomIndex)
                          {
                            ClearRoom->Widget = 0;
                            ClearRoom = RoomNext(ClearRoom);
                          }
                        }

                        room_info *RoomInfo = RoomInfoFirst(RoomList);
                        for(u32 RoomInfoIndex = 0 ;
                            RoomInfoIndex < RoomList->RoomInfoCount;
                            ++RoomInfoIndex)
                        {
                          uptr RoomSize = sizeof(room) + RoomInfo->NameLength;
                          room *Room = 0;
                          if(MemoryPoolSizeRemaining(&LocationLogged->RoomPool) >= RoomSize)
                          {
                            Room = (room *)MemoryPoolPushSize(&LocationLogged->RoomPool, RoomSize);
                          }
                          else
                          {
                            uptr SizeNew = NextPow2(LocationLogged->RoomPool.SizeTotal + RoomSize);
                            void *MemoryNew = Win32Alloc(SizeNew);
                            if(MemoryNew)
                            {
                              Win32Free(MemoryPoolCopyMove(&LocationLogged->RoomPool, MemoryNew, SizeNew));
                              Room = (room *)MemoryPoolPushSize(&LocationLogged->RoomPool, RoomSize);
                            }
                          }
                          if(!Room)
                          {
                            break;
                          }
                          else
                          {
                            MemoryZeroStruct(Room);

                            Room->ID = RoomInfo->ID;
                            Room->NameLength = RoomInfo->NameLength;
                            Room->MessagePool = MemoryPoolCreate(Win32Alloc(Megabytes(1)), Megabytes(1));
                            StringAppend(RoomInfoName(RoomInfo), RoomName(Room), RoomInfo->NameLength);

                            ++LocationLogged->RoomCount;
                            RoomInfo = RoomInfoNext(RoomInfo);
                          }
                        }
                      
                        
                        RoomListState->Timeout = 1000;
                        RoomListState->Stage = RequestStage_Cooldown;
                      } break;
                    }

                  }
                  if(!LocationLogged->CurrentRoom && LocationLogged->RoomCount)
                  {
                    LocationLogged->CurrentRoom = (room *)LocationLogged->RoomPool.Memory;
                  }

                  logged_tab TabNext = {};
                  if(LocationLogged->Tab != LoggedTab_Exit)
                  {
                    for(logged_tab CheckTabType = LoggedTab_First;
                        CheckTabType <= LoggedTab_Last;
                        CheckTabType = (logged_tab)(CheckTabType + 1))
                    {
                      if(CheckTabType != LocationLogged->Tab)
                      {
                        if(UIIsState(LocationLogged->TabBoxes[CheckTabType], UIWidgetState_Active))
                        {
                          TabNext = CheckTabType;
                          LocationLogged->TabState = TabState_NeedToFree;
                          break;
                        }
                      }
                    }
                  }
                  
                  switch(LocationLogged->Tab)
                  {
                    case LoggedTab_Messaging:
                      {
                        tab_messaging *TabMessaging = &LocationLogged->TabMessaging;
                        switch(LocationLogged->TabState)
                        {
                          case TabState_NeedToInitialize:
                            {
                              TabMessaging->SideForm = UIPushForm(WindowMain, UIFormLayout_TopToBottom, V2(0.0f, 0.1f), V2(0.2f, 0.9f));
                              {
                                UIFormSetBackgroundColor(TabMessaging->SideForm, 0xffff0000);
                                TabMessaging->DestroyRoomButton = UIPushButtonBox(TabMessaging->SideForm);
                                TabMessaging->DestroyRoomButton->Text = StringCharArray("REMOVE ROOM");
                                TabMessaging->DestroyRoomButton->TextColor = 0xff0000ff;

                                UISetFillColor(TabMessaging->DestroyRoomButton, 0xffffffff);
                                UISetWidthMin(TabMessaging->DestroyRoomButton, UISizeType_Relative, 1.0f);
                              }
                              TabMessaging->MainForm = UIPushForm(WindowMain, UIFormLayout_BottomToTop, V2(0.2f, 0.1f), V2(0.8f, 0.9f));
                              {
                                UIFormSetBackgroundColor(TabMessaging->MainForm, 0xff00ff00);

                                TabMessaging->InputBox = UIPushInputBox(TabMessaging->MainForm);
                                UISetWidthMin(TabMessaging->InputBox, UISizeType_Relative, 1.0f);
                              }
                              
                              room *ClearRoom = (room *)LocationLogged->RoomPool.Memory;
                              for(u32 RoomIndex = 0;
                                  RoomIndex < LocationLogged->RoomCount;
                                  ++RoomIndex)
                              {
                                ClearRoom->Widget = 0;
                                ClearRoom = RoomNext(ClearRoom);
                              }

                              LocationLogged->TabState = TabState_Running;
                            } break;
                          case TabState_Running:
                            {
                              {
                                request_state *DestroyRoomState = RequestStates + RRType_DestroyRoom;
                                switch(DestroyRoomState->Stage)
                                {
                                  case RequestStage_Idle:
                                    {
                                      if(LocationLogged->CurrentRoom && UIIsEvent(TabMessaging->DestroyRoomButton, UIWidgetEvent_Pressed))
                                      {
                                        socket_poll_result WritePoll = SocketPollWrite(Server);
                                        if(WritePoll.Events & POLLWRNORM)
                                        {
                                          request_destroy_room *RequestDestroyRoom = MessageHeaderRequestDestroyRoom(SendMessageHeader);
                                          RequestDestroyRoom->ID = LocationLogged->CurrentRoom->ID;

                                          SendMessage(RRType_DestroyRoom, sizeof(request_destroy_room));

                                          DestroyRoomState->Stage = RequestStage_Wait;
                                          DestroyRoomState->Timeout = 1000;
                                        }
                                      }
                                    } break;
                                }
                              }

                              // reattach rooms
                              
                              {
                                room *Room = (room *)LocationLogged->RoomPool.Memory;
                                for(u32 RoomIndex = 0;
                                    RoomIndex < LocationLogged->RoomCount;
                                    ++RoomIndex)
                                {
                                  if(!Room->Widget)
                                  {
                                    ui_room *NewWidget = UIPushRoom(TabMessaging->SideForm);
                                    if(!NewWidget)
                                    {
                                      break;
                                    }
                                    else
                                    {
                                      UISetWidthMin(NewWidget, UISizeType_Fill, 1.0f);
                                      NewWidget->Text = StringDataLength(RoomName(Room), Room->NameLength);
                                      Room->Widget = NewWidget;
                                    }
                                  }

                                  Room = RoomNext(Room);
                                }
                              }

                              {
                                request_state *ClientListState = RequestStates + RRType_ClientList;
                                switch(ClientListState->Stage)
                                {
                                  case RequestStage_Idle:
                                    {
                                      socket_poll_result WriteResult = SocketPollWrite(Server);
                                      if(WriteResult.Events & POLLWRNORM)
                                      {
                                        request_client_list *RequestClientList = MessageHeaderRequestClientList(SendMessageHeader);

                                        SendMessage(RRType_ClientList, sizeof(request_client_list));

                                        ClientListState->Stage = RequestStage_Wait;
                                        ClientListState->Timeout = 1000;
                                      }
                                    } break;

                                  case RequestStage_Ready:
                                    {
                                      response_client_list *ResponseClientList = MessageHeaderResponseClientList(RecvMessageHeader);
                                    
                                      LocationLogged->ClientBufferSize = 0; 

                                      client_info *SrcClientInfo = ClientInfoFirst(ResponseClientList);
                                      client_info *DstClientInfo = LocationLogged->FirstClientInfo;

                                      for(u32 ClientInfoIndex = 0;
                                          ClientInfoIndex < ResponseClientList->ClientInfoCount;
                                          ++ClientInfoIndex)
                                      {
                                        client_info *NextSrcClientInfo = ClientInfoNext(SrcClientInfo);
                                        uptr CopySize = (u8 *)NextSrcClientInfo  - (u8 *)SrcClientInfo;

                                        if(LocationLogged->ClientBufferSize + CopySize > LocationLogged->ClientBufferSizeMax)
                                        {
                                          break;
                                        }
                                        else
                                        {
                                          MemoryCopy(SrcClientInfo, DstClientInfo, CopySize);
                                          
                                          ++LocationLogged->ClientInfoCount;
                                          LocationLogged->ClientBufferSize += CopySize;
                                          DstClientInfo = ClientInfoNext(DstClientInfo);
                                          SrcClientInfo = NextSrcClientInfo;
                                        }

                                      }

                                      ClientListState->Timeout = 1000;
                                      ClientListState->Stage = RequestStage_Cooldown;
                                    } break;
                                }
                              }
                              room *CurrentRoom = LocationLogged->CurrentRoom;
                              {
                                request_state *SendMessageState = RequestStates + RRType_SendMessage;
                                ui_input_box *InputBox = TabMessaging->InputBox; 
                                switch(SendMessageState->Stage)
                                {
                                  case RequestStage_Idle:
                                    {
                                      if(CurrentRoom && InputBox->CanBeRead)
                                      {
                                        socket_poll_result WritePoll = SocketPollWrite(Server);
                                        if(WritePoll.Events & POLLWRNORM)
                                        {
                                          request_send_message *RequestSendMessage = MessageHeaderRequestSendMessage(SendMessageHeader);
                                          RequestSendMessage->RoomID = CurrentRoom->ID;

                                          string InputString = InputBox->CharBuffer->String;
                                          RequestSendMessage->MessageLength = InputString.Length;
                                          MemoryCopy(InputString.Data, RequestSendMessageMessage(RequestSendMessage), InputString.Length);

                                          uptr RequestSize = sizeof(request_send_message) + InputString.Length;
                                          SendMessage(RRType_SendMessage, RequestSize);

                                          SendMessageState->Stage = RequestStage_Wait;
                                          SendMessageState->Timeout = 100;
                                        }
                                      }
                                    } break;
                                  case RequestStage_Ready:
                                    {
                                      InputBox->CharBuffer->String.Length = 0;
                                      InputBox->CanBeRead = false;
                                    } break;
                                }
                              }
                              {
                                request_state *MessageListState = RequestStates + RRType_MessageList;
                                switch(MessageListState->Stage)
                                {
                                  case RequestStage_Idle:
                                    {
                                      if(CurrentRoom)
                                      {
                                        socket_poll_result WriteResult = SocketPollWrite(Server);
                                        if(WriteResult.Events & POLLWRNORM)
                                        {
                                          request_message_list *MessageList = MessageHeaderRequestMessageList(SendMessageHeader);
                                          MessageList->RoomID = CurrentRoom->ID;
                                          MessageList->OnePastLastMessageIndex = CurrentRoom->MessageCount;

                                          SendMessage(RRType_MessageList, sizeof(request_message_list));

                                          MessageListState->Data = (void *)CurrentRoom->ID;
                                          MessageListState->Stage = RequestStage_Wait;
                                          MessageListState->Timeout = 1000;
                                        }
                                      }
                                    } break;

                                  case RequestStage_Ready:
                                    {
                                      response_message_list *MessageList = MessageHeaderResponseMessageList(RecvMessageHeader);
                                      u32 DstID = (u32)*(u32 *)&MessageListState->Data;

                                      room *DstRoom = 0;
                                      {
                                        room *FindRoom = (room *)LocationLogged->RoomPool.Memory; 
                                        for(u32 RoomIndex = 0;
                                            RoomIndex < LocationLogged->RoomCount;
                                            ++RoomIndex)
                                        {
                                          if(FindRoom->ID == DstID)
                                          {
                                            DstRoom = FindRoom;
                                            break;
                                          }
                                          else
                                          {
                                            FindRoom = RoomNext(FindRoom);
                                          }
                                        }
                                      }
                                      

                                      // since room can be deleter meantime and replaced with other
                                      if(DstRoom)
                                      {
                                        memory_pool *DstMessagePool = &DstRoom->MessagePool;

                                        message_info *MessageInfo = (message_info *)(RecvBuffer + MessageList->MessageInfoOffset);
                                        for(u32 MessageIndex = 0 ;
                                            MessageIndex < MessageList->MessageInfoCount;
                                            ++MessageIndex)
                                        {
                                          uptr MessageSize = sizeof(message) + MessageInfo->StringLength;
                                          message *Message = 0;

                                          if(MemoryPoolSizeRemaining(DstMessagePool) >= MessageSize)
                                          {
                                            Message = (message *)MemoryPoolPushSize(DstMessagePool, MessageSize);
                                          }
                                          else
                                          {
                                            uptr SizeNew = NextPow2(DstMessagePool->SizeTotal + MessageSize);
                                            void *MemoryNew = Win32Alloc(SizeNew);

                                            if(MemoryNew)
                                            {
                                              Win32Free(MemoryPoolCopyMove(DstMessagePool, MemoryNew, SizeNew));

                                              UIPopAllOfType(TabMessaging->MainForm, UIWidgetType_Message);
                                              DstRoom->MessageActiveCount = 0;
                                              DstRoom->FirstMessageInactive = (message *)(DstMessagePool->Memory);

                                              Message = (message *)MemoryPoolPushSize(DstMessagePool, MessageSize);
                                            }
                                          }

                                          if(!Message)
                                          {
                                            break;
                                          }
                                          else
                                          {
                                            Message->ID = MessageInfo->ID;
                                            Message->Length = MessageInfo->StringLength;

                                            StringAppend(RecvBuffer + MessageInfo->StringOffset, (char *)(Message + 1), MessageInfo->StringLength);

                                            if(DstRoom->MessageActiveCount == DstRoom->MessageCount)
                                            {
                                              DstRoom->FirstMessageInactive = Message;
                                            }

                                            ++DstRoom->MessageCount;
                                            ++MessageInfo;
                                          }

                                        }
                                      }
                                      

                                      MessageListState->Timeout = 1000;
                                      MessageListState->Stage = RequestStage_Cooldown;
                                    } break;
                                }
                              }
                              {
                                if(CurrentRoom)
                                {
                                  while(CurrentRoom->MessageActiveCount != CurrentRoom->MessageCount)
                                  {
                                    message *UpdateMessage = CurrentRoom->FirstMessageInactive;

                                    ui_message *MessageWidget = UIPushFirstMessage(TabMessaging->MainForm);
                                    if(!MessageWidget)
                                    {
                                      break;
                                    }
                                    else
                                    {
                                      MessageWidget->Username = StringCharArray("Unknown");
                                      
                                      client_info *ClientInfo = LocationLogged->FirstClientInfo;
                                      for(u32 ClientInfoIndex = 0;
                                          ClientInfoIndex < LocationLogged->ClientInfoCount;
                                          ++ClientInfoIndex)
                                      {
                                        if(UpdateMessage->ID == ClientInfo->ID)
                                        {
                                          MessageWidget->Username = StringDataLength(ClientInfoUsername(ClientInfo), ClientInfo->UsernameLength);
                                          break;
                                        }
                                        else
                                        {
                                          ClientInfo = ClientInfoNext(ClientInfo);
                                        }
                                      }

                                      MessageWidget->Message = StringDataLength(MessageMessage(UpdateMessage), UpdateMessage->Length);
                                      CurrentRoom->FirstMessageInactive = MessageNext(CurrentRoom->FirstMessageInactive);

                                      ++CurrentRoom->MessageActiveCount;
                                    }
                                  }
                                }
                              }
                              {
                                u32 DefaultColor = 0xff8b0000;
                                u32 HoverColor = 0xff11c084;
                                u32 SelectedColor = 0xff32a219;

                                room *Room = (room *)LocationLogged->RoomPool.Memory;
                                for(u32 RoomIndex = 0;
                                    RoomIndex < LocationLogged->RoomCount;
                                    ++RoomIndex)
                                {
                                  ui_room *RoomWidget = Room->Widget;
                                  if(RoomWidget)
                                  {
                                    if(UIIsEvent(RoomWidget, UIWidgetEvent_Pressed))
                                    { 
                                      if(LocationLogged->CurrentRoom != Room)
                                      {
                                        UISetFillColor(RoomWidget, SelectedColor);

                                        UIPopAllOfType(TabMessaging->MainForm, UIWidgetType_Message);
                                        CurrentRoom->MessageActiveCount = 0;
                                        CurrentRoom->FirstMessageInactive = (message *)CurrentRoom->MessagePool.Memory;

                                        LocationLogged->CurrentRoom = Room;
                                      }
                                    }
                                    else if(UIIsEvent(RoomWidget, UIWidgetEvent_Hovered))
                                    {
                                      UISetFillColor(RoomWidget, HoverColor);
                                    }
                                    else if(Room== LocationLogged->CurrentRoom)
                                    {
                                      UISetFillColor(RoomWidget, SelectedColor);
                                    }
                                    else
                                    {
                                      UISetFillColor(RoomWidget, DefaultColor);
                                    }
                                  }

                                  Room = RoomNext(Room);
                                }
                              }
                            } break;
                          case TabState_NeedToFree:
                            { 
                              room *CurrentRoom = LocationLogged->CurrentRoom;
                              if(CurrentRoom)
                              {
                                CurrentRoom->MessageActiveCount = 0;
                                CurrentRoom->FirstMessageInactive = (message *)CurrentRoom->MessagePool.Memory;
                              }
                            } break;
                        }
                      } break;
                    case LoggedTab_CreateRoom:
                      {
                        tab_room_create *TabCreateRoom = &LocationLogged->TabCreateRoom;
                        switch(LocationLogged->TabState)
                        {
                          case TabState_NeedToInitialize:
                            {
                              TabCreateRoom->MainForm = UIPushForm(WindowMain, UIFormLayout_TopToBottom, V2(0.0f, 0.1f), V2(1.0f, 0.9f));
                              {
                                UIFormSetBackgroundColor(TabCreateRoom->MainForm, 0);

                                ui_text_box *NameText = UIPushTextBox(TabCreateRoom->MainForm);
                                NameText->Text = StringCharArray("Roomname:");
                                NameText->TextColor = 0xffffffff;

                                TabCreateRoom->RoomnameInput = UIPushInputBox(TabCreateRoom->MainForm);
                                TabCreateRoom->RoomnameInput->Filter = UIInputFilter_FirstNumberOrLetter;
                                UISetWidthMin(TabCreateRoom->RoomnameInput, UISizeType_Relative, 0.8f);

                                TabCreateRoom->CreateButton = UIPushButtonBox(TabCreateRoom->MainForm);
                                TabCreateRoom->CreateButton->Text = StringCharArray("Create");
                                TabCreateRoom->CreateButton->TextColor = 0xff000000;
                                UISetFillColor(TabCreateRoom->CreateButton, 0xffffffff);

                                TabCreateRoom->Error = UIPushTextBox(TabCreateRoom->MainForm);
                                TabCreateRoom->ErrorBufferSize = 4096;
                                TabCreateRoom->ErrorBuffer = (char *)MemoryPoolPushSize(&LocationLogged->TabTransientPool, TabCreateRoom->ErrorBufferSize);

                                LocationLogged->TabState = TabState_Running;
                              }
                            } break;
                          case TabState_Running:
                            {
                              request_state *CreateRoomState = RequestStates + RRType_CreateRoom;
                              switch(CreateRoomState->Stage)
                              {
                                case RequestStage_Idle:
                                  {
                                    if(UIIsEvent(TabCreateRoom->CreateButton, UIWidgetEvent_Pressed))
                                    {
                                      string Roomname = TabCreateRoom->RoomnameInput->CharBuffer->String;

                                      if(Roomname.Length == 0)
                                      {
                                        TabCreateRoom->Error->Text = StringCharArray("Roomname is empty");
                                        TabCreateRoom->Error->TextColor = 0xff0000ff;
                                      }
                                      else
                                      {
                                        socket_poll_result WriteResult = SocketPollWrite(Server);
                                        if(!WriteResult.State)
                                        {
                                          TabCreateRoom->Error->Text = StringCharArray("Pressed button at the wrong time lol...");
                                          TabCreateRoom->Error->TextColor = 0xff0000ff;
                                        }
                                        else
                                        {
                                          request_create_room *RequestCreateRoom = MessageHeaderRequestCreateRoom(SendMessageHeader);

                                          RequestCreateRoom->NameLength = Roomname.Length;

                                          MemoryCopy(Roomname.Data, RequestCreateRoomName(RequestCreateRoom), Roomname.Length);

                                          uptr RequestSize = sizeof(request_create_room) + Roomname.Length;
                                          SendMessage(RRType_CreateRoom, RequestSize);

                                          TabCreateRoom->Error->Text = StringCharArray("Awaiting server...");
                                          TabCreateRoom->Error->TextColor = 0xff00a5ff;

                                          CreateRoomState->Stage = RequestStage_Wait;
                                          CreateRoomState->Timeout = 3000;
                                        }

                                      }
                                    }
                                  } break;

                                case RequestStage_TimedOut:
                                {
                                  TabCreateRoom->Error->Text = StringCharArray("Server timed out, try again...");
                                  TabCreateRoom->Error->TextColor = 0xff0000ff;
                                } break;
                                case RequestStage_Ready:
                                  {
                                    response_create_room *ResponseCreateRoom = MessageHeaderResponseCreateRoom(RecvMessageHeader);
                                    ui_char_buffer *CharBuffer = TabCreateRoom->RoomnameInput->CharBuffer;
                                    string *Roomname = &CharBuffer->String;
                                    switch(ResponseCreateRoom->Error)
                                    {
                                      case ResponseCreateRoomError_None:
                                        {
                                          u32 ErrorBufferLength = StringFormat(TabCreateRoom->ErrorBuffer, TabCreateRoom->ErrorBufferSize,
                                              "Success: Room with name \"(s)\" was created successfully!", *Roomname);
                                          TabCreateRoom->Error->Text = StringDataLength(TabCreateRoom->ErrorBuffer, ErrorBufferLength);
                                          TabCreateRoom->Error->TextColor = 0xff00ff00;

                                          TabCreateRoom->RoomnameInput->CharBuffer->String.Length = 0;
                                        } break;
                                      case ResponseCreateRoomError_RoomnameExists:
                                        {
                                          TabCreateRoom->Error->Text = StringCharArray("Error: Roomname is taken");
                                          TabCreateRoom->Error->TextColor = 0xff0000ff;
                                        } break;
                                      case ResponseCreateRoomError_TooManyRooms:
                                        {
                                          TabCreateRoom->Error->Text = StringCharArray("Error: Too many rooms");
                                          TabCreateRoom->Error->TextColor = 0xff0000ff;
                                        } break;
                                    }
                                    Roomname->Length = 0;
                                  } break;
                              }
                            } break;
                          case TabState_NeedToFree:
                            {
                            } break;
                        }
                      } break;
                    case LoggedTab_Exit:
                      {
                        request_state *LogoutState = RequestStates + RRType_Logout;
                        switch(LogoutState->Stage)
                        {
                          case RequestStage_Idle:
                            {
                              socket_poll_result WritePoll = SocketPollWrite(Server);
                              if(WritePoll.Events & POLLWRNORM)
                              {
                                SendMessage(RRType_Logout, sizeof(request_logout));
                                LogoutState->Stage = RequestStage_Wait;
                                LogoutState->Timeout = 100;
                              }
                            } break;
                          case RequestStage_Ready:
                            {
                              UIPopAll(WindowMain);
                              SetLocation(&MenuContext, MenuLocation_Start);
                              MemoryZeroStruct(&RequestStates);
                            } break;
                        }
                      } break;
                      InvalidDefaultCase;
                  }

                  if(LocationLogged->TabState == TabState_NeedToFree)
                  {
                    LocationLogged->TabState = TabState_NeedToInitialize;
                    LocationLogged->Tab = TabNext;
                    UIPopSubsequentForms(LocationLogged->TopForm);
                    MemoryPoolReset(&LocationLogged->TabTransientPool);
                  }

                }
              } break;
              InvalidDefaultCase;
          }
          for(rr_type RequestType = RRType_First;
              RequestType <= RRType_Last;
              RequestType = (rr_type)(RequestType + 1))
          {
            request_state *RequestState = RequestStates + RequestType;

            switch(RequestState->Stage)
            {
              case RequestStage_Wait:
                {
                  if(RequestState->Timeout == TimeoutInfinite)
                  {
                  }
                  if(RequestState->Timeout > DeltaMs)
                  {
                    RequestState->Timeout -= DeltaMs;
                  }
                  else
                  {
                    RequestState->Timeout = 0;
                    RequestState->Stage = RequestStage_TimedOut;
                  }
                } break;
              case RequestStage_TimedOut:
                {
                  RequestState->Stage = RequestStage_Idle;
                } break;
              case RequestStage_Ready:
                {
                  RequestState->Stage = RequestStage_Idle;
                } break;
              case RequestStage_Cooldown:
                {
                  if(RequestState->Timeout == TimeoutInfinite)
                  {
                  }
                  if(RequestState->Timeout > DeltaMs)
                  {
                    RequestState->Timeout -= DeltaMs;
                  }
                  else
                  {
                    RequestState->Timeout = 0;
                    RequestState->Stage = RequestStage_Idle;
                  }
                } break;
            }
          }
          switch(ConnectionStatus)
          {
            case ConnectionStatus_Disconnected:
              {
                // TODO should loop through all infos!
                if(Server == INVALID_SOCKET)
                {
                  Server = WSASocketW(ServerAddressInfo->ai_family, ServerAddressInfo->ai_socktype, ServerAddressInfo->ai_protocol, 0, 0, WSA_FLAG_OVERLAPPED);
                  u_long Mode = ~0u;
                  ioctlsocket(Server, FIONBIO, &Mode);
                }

                WSAConnect(Server, ServerAddressInfo->ai_addr, (int)ServerAddressInfo->ai_addrlen, 0, 0, 0, 0);
                s32 ConnectionError = WSAGetLastError();

                switch(ConnectionError)
                {
                  case WSAEISCONN:
                    {
                      ConnectionStatus = ConnectionStatus_Connected;
                    } break;
                  case WSAECONNREFUSED:
                  case WSAENETUNREACH:
                  case WSAETIMEDOUT:
                    {
                      ConnectionStatus = ConnectionStatus_Disconnected;
                    } break;
                  default:
                    {
                      // TODO some error handling?
                      ConnectionStatus = ConnectionStatus_Disconnected;
                    } break;
                }
              } break;
            case ConnectionStatus_Connected:
              {
                socket_poll_result ReadPoll = SocketPollReadWithErrors(Server);
                if(ReadPoll.State == SOCKET_ERROR || ReadPoll.Events & (POLLERR | POLLHUP))
                {
                  closesocket(Server);
                  Server = INVALID_SOCKET;
                  ConnectionStatus = ConnectionStatus_Disconnected;
                  MemoryZeroStruct(&RequestStates);
                }
                // listen to server
                else if(ReadPoll.Events & POLLRDNORM)
                {
                  WSABUF Buffer;
                  Buffer.len = Message_SizeMax;
                  Buffer.buf = (char *)RecvMessageHeader;

                  DWORD Flags = 0;
                  b32 ReceiveResult = WSARecv(Server, &Buffer, 1, (DWORD *)&RecvSize, &Flags, 0, 0);

                  if(MessageHeaderIsValid(RecvMessageHeader)) 
                  {
                    request_state *RequestState = RequestStates + RecvMessageHeader->RRType;
                    if(RequestState->Stage == RequestStage_Wait)
                    {
                      RequestState->Stage = RequestStage_Ready;
                    }
                    else
                    {
                      // TODO we missed something?
                    }
                  }

                }
              } break;
          }
          
          

          UINextFrame(WindowMain, &Font);
          

          {
            // reset input
            WindowNative->ScrollOffset = 0;
            // 
            //GlobalWindow.InputLength = 0;
            MSG Msg;
            while(PeekMessageA(&Msg, 0, 0, 0, PM_REMOVE))
            {
              switch(Msg.message)
              {
                case WM_SYSKEYDOWN:
                case WM_KEYDOWN:
                  {
                    u32 Keycode = (u32)Msg.wParam;
                    switch(Keycode)
                    {
                      case VK_BACK:
                        {
                          ui_widget *ActiveWidget = WindowMain->WidgetWithState[UIWidgetState_Active];
                          if(ActiveWidget)
                          {
                            switch(ActiveWidget->Type)
                            {
                              case UIWidgetType_InputBox:
                                {
                                  UIPopKeycode(UIGetInputBox(ActiveWidget));
                                } break;
                              case UIWidgetType_ButtonBox:
                              case UIWidgetType_TextBox:
                                {
                                } break;
                                InvalidDefaultCase;
                            }
                          }

                        } break;

                      case VK_RETURN:
                        {
                          ui_widget *ActiveWidget = WindowMain->WidgetWithState[UIWidgetState_Active];
                          if(ActiveWidget)
                          {
                            switch(ActiveWidget->Type)
                            {
                              case UIWidgetType_InputBox:
                                {
                                  ui_input_box *InputBox = UIGetInputBox(ActiveWidget);
                                  if(InputBox->CharBuffer->String.Length)
                                  {
                                    InputBox->CanBeRead = true;
                                  }
                                } break;
                              case UIWidgetType_ButtonBox:
                              case UIWidgetType_TextBox:
                                {
                                } break;
                                InvalidDefaultCase;
                            }
                          }
                        } break;
                      case VK_ESCAPE:
                        {
                          ui_widget *ActiveWidget = WindowMain->WidgetWithState[UIWidgetState_Active];
                          if(ActiveWidget)
                          {
                            UIResetWidgetState(WindowMain, ActiveWidget, UIWidgetState_Active);
                          }
                        } break;
                      default:
                        {
                          TranslateMessage(&Msg);
                          DispatchMessage(&Msg);
                        } break;
                    }
                  } break;
                default:
                  {
                    TranslateMessage(&Msg);
                    DispatchMessage(&Msg);
                  } break;
              } 
            }
          }
          // update screen
          {
            HDC WDC = GetDC(WindowNative->Handle);
            RECT WindowRect;
            GetClientRect(WindowNative->Handle, &WindowRect);

            u32 WindowWidth = WindowRect.right - WindowRect.left;
            u32 WindowHeight = WindowRect.bottom - WindowRect.top;
            if(WindowNative->Bitmap.Width < WindowWidth)
              PatBlt(WDC, WindowNative->Bitmap.Width, 0, WindowWidth - WindowNative->Bitmap.Width, WindowHeight, 0);
            if(WindowNative->Bitmap.Height < WindowHeight)
              PatBlt(WDC, 0, WindowNative->Bitmap.Height, WindowNative->Bitmap.Width, WindowHeight - WindowNative->Bitmap.Height, 0);

            u32 *SrcRow = WindowMain->Bitmap.Pixels;
            u32 *DstRow = WindowNative->Bitmap.Pixels;

            u32 RowCount = Min(WindowNative->Bitmap.Height, WindowMain->Bitmap.Height);
            u32 ColCount = Min(WindowNative->Bitmap.Width, WindowMain->Bitmap.Width);

            for(u32 RowIndex = 0;
                RowIndex < RowCount;
                ++RowIndex)
            {
              u32 *Src = SrcRow;
              u32 *Dst = DstRow;

              for(u32 ColIndex = 0;
                  ColIndex < ColCount;
                  ++ColIndex)
              {
                u32 A = (*Src >> 24) & 0xff;
                u32 R = (*Src >> 0) & 0xff;
                u32 G = (*Src >> 8) & 0xff;
                u32 B = (*Src >> 16) & 0xff;
                *Dst = (B << 0) | (G << 8) | (R << 16) | (A << 24);

                ++Src;
                ++Dst;
              }

              SrcRow += WindowMain->Bitmap.Pitch;
              DstRow += WindowNative->Bitmap.Pitch;
            }

            BITMAPINFO BitmapInfo = {};
            BitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            BitmapInfo.bmiHeader.biPlanes = 1;
            BitmapInfo.bmiHeader.biBitCount = 32;
            BitmapInfo.bmiHeader.biCompression = BI_RGB;
            BitmapInfo.bmiHeader.biWidth = (LONG)WindowNative->Bitmap.Width;
            BitmapInfo.bmiHeader.biHeight = -(LONG)WindowNative->Bitmap.Height;

            StretchDIBits(
                WDC,
                0, 0, WindowWidth, WindowHeight,
                0, 0, WindowNative->Bitmap.Width, WindowNative->Bitmap.Height,
                WindowNative->Bitmap.Pixels,
                &BitmapInfo,
                DIB_RGB_COLORS,
                SRCCOPY);
            ReleaseDC(WindowNative->Handle, WDC);
          }

          LARGE_INTEGER CurrentTime;
          QueryPerformanceCounter(&CurrentTime);
          u64 ElapsedTime = (u64)(CurrentTime.QuadPart - LastTime.QuadPart);

          DeltaMs = (ElapsedTime * 1000) / (u64)Frequency.QuadPart;
          LastTime = CurrentTime;
        }
      }
      WSACleanup();
    }
  }
  return Result;
}
