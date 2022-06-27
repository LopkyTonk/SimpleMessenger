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


#define SaveFilename "server_data.d"

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

struct client
{
  SOCKET Socket;
  u64 CredentialOffset;
};
struct credential
{
  u32 ID;
  u32 UsernameLength;
  u32 PasswordLength;
};
credential *
CredentialNext(credential *Credential)
{
  uptr ByteOffset = sizeof(credential) + Credential->UsernameLength + Credential->PasswordLength;
  credential *Result = (credential *)((u8 *)Credential + ByteOffset);
  return Result;
}
char *
CredentialUsername(credential *Credential)
{
  uptr ByteOffset = sizeof(credential);
  char *Result = (char *)((u8 *)Credential + ByteOffset);
  return Result;
}
char *
CredentialPassword(credential *Credential)
{
  uptr ByteOffset = sizeof(credential) + Credential->UsernameLength;
  char *Result = (char *)((u8 *)Credential + ByteOffset);
  return Result;
}

struct buffer
{
  void *Buffer;
  uptr BufferSizeMax;
  uptr BufferSize;

  u32 Count;
};

struct client_message
{
  u32 ID;
  u32 Length;
  // char Message[Length]
};
#define ClientMessageNext(ClientMessage)((client_message *)((u8 *)ClientMessage + sizeof(client_message) + ClientMessage->Length))
#define ClientMessageMessage(ClientMessage)((char *)(ClientMessage + 1))

struct room
{
  u32 ID;
  u32 NameLength;
  
  memory_pool MessagePool;
  u32 MessageCount;
};

room *
RoomNext(room *Room)
{
  uptr ByteOffset = sizeof(room) + Room->NameLength;
  room *Result = (room *)((u8 *)Room + ByteOffset);
  return Result;
}

char *
RoomName(room *Room)
{
  uptr ByteOffset = sizeof(room);
  char *Result = (char *)((u8 *)Room + ByteOffset);
  return Result;
}

#if 0
client_message *
RoomFirstClientMessage(room *Room)
{
  uptr ByteOffset = sizeof(room) + Room->NameLength;
  client_message *Result = (client_message *)((u8 *)Room + ByteOffset);
  return Result;
}
client_message *
RoomFirstFreeClientMessage(room *Room)
{
  uptr ByteOffset = sizeof(room) + Room->NameLength + Room->BufferSize;
  client_message *Result = (client_message *)((u8 *)Room + ByteOffset);
  return Result;
}
#endif 

room *
RoomFind(room *Rooms, u32 RoomCount, u32 ID)
{
  room *Result = 0;

  room *Room = Rooms;
  for(u32 RoomIndex = 0;
      RoomIndex < RoomCount;
      ++RoomIndex)
  {
    if(ID == Room->ID)
    {
      Result = Room;
      break;
    }

    Room = RoomNext(Room);
  }

  return Result;
}

#define FileHeaderMagicValue (u64(0x11223344556677))
struct file_header
{
  u64 MagicValue;

  u32 CredentialCount;
  u32 RoomCount;
  
  u64 CredentialOffset;
  u64 RoomOffset;

  u64 CredentialSize;
  u64 RoomnameSize;
};

struct file_room
{
  u32 NameLength;
  u32 MessageCount;

  u64 NameOffset;
  u64 MessageOffset;
  u64 MessageSize;
};


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

int WinMain(HINSTANCE Instance,
    HINSTANCE PrevInstante,
    LPSTR CmdLine, 
    int ShowCmd)

{
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
  int Result = 0;

  char *SendBuffer = (char *)Win32Alloc(Message_SizeMax * 2);
  char *RecvBuffer = (char *)((u8 *)SendBuffer + Message_SizeMax);

  message_header *SendMessageHeader = (message_header *)(SendBuffer);
  message_header *RecvMessageHeader = (message_header *)(RecvBuffer);

  SendMessageHeader->MagicValue = MessageHeaderMagicValue;

  WSAOVERLAPPED SendOverlapped = {};
  auto SendMessage = [&SendMessageHeader, &SendOverlapped](SOCKET Client, rr_type RRType, uptr ResponseSize)
  {
    SendMessageHeader->RRType = RRType;

    WSABUF Buffer;
    Buffer.len = (ULONG)(sizeof(message_header) + ResponseSize);
    Buffer.buf = (char *)SendMessageHeader;

    DWORD Ignored;
    s32 SendResult = WSASend(Client, &Buffer, 1, &Ignored, 0, &SendOverlapped, 0);

    WSAWaitForMultipleEvents(1, &SendOverlapped.hEvent, true, INFINITE, true);
  };

  // setup windows
  win32_window WindowNative_ = {};
  WindowNative = &WindowNative_;
  WindowNative->WidthMax = 1920;
  WindowNative->HeightMax = 1080;
  u32 BitmapSize = sizeof(u32) * WindowNative->WidthMax * WindowNative->HeightMax;
  WindowNative->Bitmap = Bitmap((u32 *)Win32Alloc(BitmapSize), 0, 0);

  u32 WindowMemorySize = Megabytes(50);
  void *Memory = VirtualAlloc((void *)Terabytes(2), WindowMemorySize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
  ui_window WindowMain_ = UIWindow(Memory, WindowMemorySize, 1920, 1080, 0xff000000);
  WindowMain = &WindowMain_;

  char ClassName[] = "WINDOW_SERVER";
  WNDCLASS WC = {};
  WC.lpfnWndProc = Win32WindowProc;
  WC.hInstance = Instance;
  WC.lpszClassName = ClassName;
  WC.hCursor = LoadCursor(NULL, IDC_ARROW); 

  RegisterClass(&WC);

  WindowNative->Handle = CreateWindowEx(0, ClassName, "server", WS_OVERLAPPEDWINDOW, 
      CW_USEDEFAULT,CW_USEDEFAULT,
      CW_USEDEFAULT,CW_USEDEFAULT,
      0, 0, Instance, 0);
  ShowWindow(WindowNative->Handle, SW_SHOW);

  if(!WindowNative->Handle)
  {
    Result = -1;
  }
  else
  {
    WindowNative->Quit = false;

    ui_form *MainForm = UIPushForm(WindowMain, UIFormLayout_BottomToTop, V2(0.0f, 0.0f), V2(1.0f, 1.0f));
    auto OutputTextString = [&](string Text)
    {
      ui_text_box *TextBox = UIPushFirstTextBox(MainForm);
      TextBox->Text = Text;
      TextBox->TextColor = 0xffffffff;
    };
#define OutputTextCharArray(CharArray)(OutputTextString(StringCharArray(CharArray)))

    WORD WSAVersion = MAKEWORD(2, 2); 
    WSADATA WSAData;
    DWORD StartupResult = WSAStartup(WSAVersion, &WSAData);
    if(StartupResult != 0)
    {
      OutputTextCharArray("[SERVER]: Failed to startup WSA!");
      Result = StartupResult;
    }
    else
    {
      SendOverlapped.hEvent = WSACreateEvent();
      if(SendOverlapped.hEvent == 0)
      {
        OutputTextCharArray("[SERVER]: Failed to crete event!");
      }
      else
      {

        ADDRINFOA Hints = {};
        Hints.ai_family = AF_UNSPEC;
        Hints.ai_socktype = SOCK_STREAM;
        Hints.ai_protocol = IPPROTO_TCP;
        Hints.ai_flags = AI_PASSIVE;

        ADDRINFOA *AddressInfo;
        DWORD GetAddrResult = getaddrinfo(0, "12121", &Hints, &AddressInfo);
        if(GetAddrResult != 0)
        {
          OutputTextCharArray("[SERVER]: Failed to get addres info!");
          Result = GetAddrResult;
        }
        else
        {
          SOCKET ListenSocket = socket(AddressInfo->ai_family, AddressInfo->ai_socktype, AddressInfo->ai_protocol);
          if(ListenSocket == SOCKET_ERROR)
          {
            OutputTextCharArray("[SERVER]: Failed to connect socket!");
            closesocket(ListenSocket);
            Result = WSAGetLastError();
          } 
          else
          {
            OutputTextCharArray("[SERVER]: Attempting to bind!");
            DWORD BindResult = bind(ListenSocket, AddressInfo->ai_addr, static_cast<int>(AddressInfo->ai_addrlen));
            if(BindResult == SOCKET_ERROR)
            {
              Result = WSAGetLastError();
              OutputTextCharArray("[SERVER]: Failed to bind socket!");
            }
            else
            {
              OutputTextCharArray("[SERVER]: Successfuly binded!");
              freeaddrinfo(AddressInfo);

              DWORD ListenResult = listen(ListenSocket, SOMAXCONN);
              OutputTextCharArray("[SERVER]: Attempting to listen!");
              if(ListenResult == SOCKET_ERROR)
              {
                OutputTextCharArray("[SERVER]: Failed to listen socket!");
                Result = WSAGetLastError();
              }
              else
              { 
                OutputTextCharArray("[SERVER]: Succeded to listen!");
                
                uptr DefaultSize = Megabytes(1);
                memory_pool ClientPool = MemoryPoolCreate(Win32Alloc(DefaultSize), DefaultSize);
                memory_pool CredentialPool;
                memory_pool RoomPool;

                u32 ClientCount = 0;
                // since we dont delete any
                u32 CredentialCount = 0;
                u32 RoomCount = 0;
                u32 RoomID = 1;

                auto DefaultPoolInit = [&]()
                {
                  CredentialPool = MemoryPoolCreate(Win32Alloc(DefaultSize), DefaultSize);
                  RoomPool = MemoryPoolCreate(Win32Alloc(DefaultSize), DefaultSize);
#ifdef _DEBUG
                  {
                    credential *Credential = (credential *)MemoryPoolPushSize(&CredentialPool, sizeof(credential) + 2);
                    Credential->ID = ++CredentialCount;
                    Credential->UsernameLength = Credential->PasswordLength = 1;
                    *CredentialUsername(Credential) = *CredentialPassword(Credential) = 'a';
                  }
#endif 
                };
                
                {
                  HANDLE FileHandle = CreateFileA(SaveFilename, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, 0);
                  if(FileHandle == INVALID_HANDLE_VALUE)
                  {
                    DefaultPoolInit();
                  }
                  else
                  {
                    auto ReadFileData = [&FileHandle](u64 Offset, void *Dst, u32 DstSize)
                    {
                      LONG OffsetLow = (LONG)Offset;
                      LONG OffsetHigh = (LONG)(Offset >> 32);

                      SetFilePointer(FileHandle, OffsetLow, &OffsetHigh, 0);

                      DWORD Ignored;
                      ReadFile(FileHandle, Dst, DstSize, &Ignored, 0);
                    };

                    /*
                     * Header
                     * credential_info
                     * Room
                     * Usernames and passwords
                     * Roomnames 
                     * Messages
                     */
                    file_header FileHeader;
                    ReadFileData(0, &FileHeader, sizeof(FileHeader));
                    if(FileHeader.MagicValue != FileHeaderMagicValue)
                    {
                      DefaultPoolInit();
                    }
                    else
                    {
                      {
                        uptr CredentialSize = FileHeader.CredentialSize;
                        uptr CredentialSize2 = NextPow2(CredentialSize);
                        CredentialPool = MemoryPoolCreate(Win32Alloc(CredentialSize2), CredentialSize2);
                        CredentialCount = FileHeader.CredentialCount;

                        ReadFileData(FileHeader.CredentialOffset, MemoryPoolPushSize(&CredentialPool, CredentialSize), (u32)CredentialSize);
                        //TODO copy
                      }
                      {
                        uptr RoomSize = NextPow2(sizeof(room) * FileHeader.RoomCount + FileHeader.RoomnameSize);
                        RoomPool = MemoryPoolCreate(Win32Alloc(RoomSize), RoomSize);
                        RoomCount = FileHeader.RoomCount;
                        RoomID = FileHeader.RoomCount + 1;

                        uptr FileRoomOffset = FileHeader.RoomOffset;
                        //= (file_room *)(FileMemory + FileHeader.RoomOffset);

                        for(u32 RoomIndex = 0;
                            RoomIndex < FileHeader.RoomCount;
                            ++RoomIndex)
                        {
                          file_room FileRoom;
                          ReadFileData(FileRoomOffset, &FileRoom, sizeof(FileRoom));

                          room *Room = (room *)MemoryPoolPushSize(&RoomPool, sizeof(room) + FileRoom.NameLength);

                          Room->NameLength = FileRoom.NameLength;
                          ReadFileData(FileRoom.NameOffset, RoomName(Room), FileRoom.NameLength);

                          Room->ID = RoomIndex + 1;

                          memory_pool *MessagePool = &Room->MessagePool;
                          uptr MessagePoolSize = FileRoom.MessageSize;
                          uptr MessagePoolSize2 = NextPow2(MessagePoolSize);
                              UIPopAllOfType(WindowMain, UIWidgetType_Room);
                          *MessagePool = MemoryPoolCreate(Win32Alloc(MessagePoolSize2), MessagePoolSize2);

                          ReadFileData(FileRoom.MessageOffset, MemoryPoolPushSize(MessagePool, MessagePoolSize), (u32)MessagePoolSize);
                          Room->MessageCount = FileRoom.MessageCount;

                          FileRoomOffset += sizeof(file_room);
                        }
                      }
                    }
                    CloseHandle(FileHandle);
                  }
                }
                

                while(!WindowNative->Quit)
                {
                  WSAPOLLFD PollListen;
                  PollListen.fd = ListenSocket;
                  PollListen.events = POLLRDNORM;

                  // check for new connections
                  b32 CheckForNewConnections = true;
                  while(CheckForNewConnections)
                  {
                    socket_poll_result ReadResult = SocketPollRead(ListenSocket);

                    switch(ReadResult.State)
                    {
                      case SOCKET_ERROR:
                        {
                          CheckForNewConnections = false;
                        } break;
                      case 0:
                        {
                          CheckForNewConnections = false;
                        } break;
                      case 1:
                        {
                          if(ReadResult.Events & POLLRDNORM)
                          {
                            SOCKET NewSocket = accept(ListenSocket, 0, 0);
                            if(NewSocket == SOCKET_ERROR)
                            {
                              closesocket(NewSocket);
                            }
                            else
                            {
                              client *NewClient = MemoryPoolTryPushStruct(&ClientPool, client);
                              if(NewClient)
                              {
                                MemoryZeroStruct(NewClient);

                                NewClient->Socket = NewSocket;
                                ++ClientCount;
                                OutputTextCharArray("[SERVER]: Client has just connected!");
                              }
                            }
                          }
                        } break;
                    }
                  }


                  // ask if any clients disconnected
                  for(u32 SocketIndex = 0;
                      SocketIndex < ClientCount;)
                  {
                    client *Client = (client *)ClientPool.Memory + SocketIndex;
                    WSAPOLLFD PollSocket;
                    PollSocket.fd = Client->Socket;
                    PollSocket.events = 0;

                    switch(WSAPoll(&PollSocket, 1, 0))
                    {
                      case SOCKET_ERROR:
                        {
                          ++SocketIndex;
                        } break;
                      case 0:
                        {
                          ++SocketIndex;
                        } break;
                      case 1:
                        {
                          if(PollSocket.revents & POLLHUP)
                          {
                            OutputTextCharArray("[SERVER]: Client has just disconnected!");
                            closesocket(PollSocket.fd);
                            
                            client *LastClient = (client *)ClientPool.Memory + ClientCount - 1;
                            *Client = *LastClient;
                            MemoryPoolPopStruct(&ClientPool, client);

                            --ClientCount;
                          }
                          else
                          {
                            ++SocketIndex;
                          }
                        } break;
                    }
                  }
                     

                  // check for requests
                  {
                    // TODO maybe don't do one by one?
                    for(u32 ClientIndex = 0;
                        ClientIndex < ClientCount;
                        ++ClientIndex)
                    {
                      client *Client = (client *)ClientPool.Memory + ClientIndex;
                      WSAPOLLFD Poll;
                      Poll.fd = Client->Socket;
                      Poll.events = POLLRDNORM;
                      s32 PollCount = WSAPoll(&Poll, 1, 0);

                      u32 Error = WSAGetLastError();

                      switch(PollCount)
                      {
                        case SOCKET_ERROR:
                          {
                          } break;
                        case 0:
                          {
                          } break;
                        case 1:
                          {
                            DWORD ReceivedSize;
                            WSABUF ReceiveBuffer;
                            ReceiveBuffer.len = Message_SizeMax;
                            ReceiveBuffer.buf = (char *)RecvMessageHeader;
                            DWORD Flags = 0;

                            b32 ReceiveResult = WSARecv(Client->Socket, &ReceiveBuffer, 1, &ReceivedSize, &Flags, 0, 0);

                            if(ReceiveResult == SOCKET_ERROR)
                            {
                            }
                            else
                            {
                              if(MessageHeaderIsValid(RecvMessageHeader))
                              {
                                switch(RecvMessageHeader->RRType)
                                {
                                  case RRType_CreateAccount:
                                    {
                                      request_create_account *RequestCreateAccount = MessageHeaderRequestCreateAccount(RecvMessageHeader);

                                      string Username = StringDataLength(RequestCreateAccountUsername(RequestCreateAccount), RequestCreateAccount->UsernameLength);
                                      string Password = StringDataLength(RequestCreateAccountPassword(RequestCreateAccount), RequestCreateAccount->PasswordLength);

                                      b32 UsernameExists = false;
                                      {
                                        credential *CheckCredential = (credential *)CredentialPool.Memory;
                                        for(u32 CredentialIndex = 0;
                                            CredentialIndex < CredentialCount;
                                            ++CredentialIndex)
                                        {
                                          string CheckUsername = StringDataLength(CredentialUsername(CheckCredential), CheckCredential->UsernameLength);
                                          if(StringEqual(Username, CheckUsername))
                                          {
                                            UsernameExists = true;
                                            break;
                                          }

                                          CheckCredential = CredentialNext(CheckCredential);
                                        }
                                      }

                                      response_create_account *ResponseCreateAccount = MessageHeaderResponseCreateAccount(SendMessageHeader);
                                      if(UsernameExists)
                                      {
                                        ResponseCreateAccount->Error = ResponseCreateAccountError_UsernameExists;
                                      }
                                      else
                                      {
                                        uptr RequiredSize = sizeof(credential) + Username.Length + Password.Length;
                                        credential *Credential = 0;
                                        if(MemoryPoolSizeRemaining(&CredentialPool) >= RequiredSize)
                                        {
                                          Credential = (credential *)MemoryPoolPushSize(&CredentialPool, RequiredSize);
                                        }
                                        else
                                        {
                                          uptr SizeNew = NextPow2(CredentialPool.SizeTotal + RequiredSize);
                                          void *MemoryNew = Win32Alloc(SizeNew);
                                          if(MemoryNew)
                                          {
                                            Win32Free(MemoryPoolCopyMove(&CredentialPool, MemoryNew, SizeNew));
                                            Credential = (credential *)MemoryPoolPushSize(&CredentialPool, RequiredSize);
                                          }
                                        }


                                        if(!Credential)
                                        {
                                          ResponseCreateAccount->Error = ResponseCreateAccountError_TooManyAccounts;
                                        }
                                        else
                                        {
                                          ResponseCreateAccount->Error = ResponseCreateAccountError_None;

                                          Credential->ID = ++CredentialCount;
                                          Credential->UsernameLength = Username.Length;
                                          Credential->PasswordLength = Password.Length;
                                          StringAppend(Username.Data, CredentialUsername(Credential), Username.Length);
                                          StringAppend(Password.Data, CredentialPassword(Credential), Password.Length);
                                        }
                                        SendMessage(Client->Socket, RRType_CreateAccount, sizeof(response_create_account)); 
                                      }
                                    } break;
                                  case RRType_Login:
                                    {
                                      request_login *RequestLogin = MessageHeaderRequestLogin(RecvMessageHeader);
                                      response_login *ResponseLogin = MessageHeaderResponseLogin(SendMessageHeader);
                                      ResponseLogin->Error = ResponseLoginError_InvalidUsernameOrPassword;


                                      string Username = StringDataLength(RequestLoginUsername(RequestLogin), RequestLogin->UsernameLength);
                                      string Password = StringDataLength(RequestLoginPassword(RequestLogin), RequestLogin->PasswordLength);

                                      credential *FirstCredential = (credential *)CredentialPool.Memory;
                                      credential *Credential = FirstCredential;
                                      for(u32 CredentialIndex = 0;
                                          CredentialIndex < CredentialCount;
                                          ++CredentialIndex)
                                      {
                                        string CheckUsername = StringDataLength(CredentialUsername(Credential), Credential->UsernameLength);
                                        string CheckPassword = StringDataLength(CredentialPassword(Credential), Credential->PasswordLength);

                                        if(StringEqual(Username, CheckUsername) && StringEqual(Password, CheckPassword))
                                        {
                                          ResponseLogin->Error = ResponseLoginError_None;
                                          Client->CredentialOffset = (u64)MemoryByteOffset(FirstCredential, Credential);
                                          break;
                                        }

                                        Credential = CredentialNext(Credential);
                                      }

                                      SendMessage(Client->Socket, RRType_Login, sizeof(response_login)); 
                                    } break;
                                  case RRType_Logout:
                                    {
                                      Client->CredentialOffset = U64Max;
                                      SendMessage(Client->Socket, RRType_Logout, sizeof(response_logout));
                                    } break;
                                  case RRType_CreateRoom:
                                    {
                                      request_create_room *RequestCreateRoom = MessageHeaderRequestCreateRoom(RecvMessageHeader);

                                      string Roomname = StringDataLength(RequestCreateRoomName(RequestCreateRoom), RequestCreateRoom->NameLength);
                                      b32 RoomnameExists = false;
                                      {
                                        room *CheckRoom = (room *)RoomPool.Memory;
                                        for(u32 RoomIndex = 0;
                                            RoomIndex < RoomCount;
                                            ++RoomIndex)
                                        {
                                          string CheckRoomname = StringDataLength(RoomName(CheckRoom), CheckRoom->NameLength);
                                          if(StringEqual(Roomname, CheckRoomname))
                                          {
                                            RoomnameExists = true;
                                            break;
                                          }

                                          CheckRoom = RoomNext(CheckRoom);
                                        }
                                      }

                                      response_create_room *ResponseCreateRoom = MessageHeaderResponseCreateRoom(SendMessageHeader);
                                      if(RoomnameExists)
                                      {
                                        ResponseCreateRoom->Error = ResponseCreateRoomError_RoomnameExists;
                                      }
                                      else
                                      {
                                        uptr MessageBufferSizeMax = Megabytes(10);
                                        uptr RequiredSize = sizeof(room) + Roomname.Length;
                                        room *Room = 0;
                                        if(MemoryPoolSizeRemaining(&RoomPool) >= RequiredSize)
                                        {
                                          Room = (room *)MemoryPoolPushSize(&RoomPool, RequiredSize);
                                        }
                                        else
                                        {
                                          uptr SizeNew = NextPow2(RoomPool.SizeTotal + RequiredSize);
                                          void *MemoryNew = Win32Alloc(SizeNew);
                                          if(MemoryNew)
                                          {
                                            MemoryPoolCopyMove(&RoomPool, MemoryNew, SizeNew);
                                            Room = (room *)MemoryPoolPushSize(&RoomPool, RequiredSize);
                                          }
                                        }

                                        if(!Room)
                                        {
                                          ResponseCreateRoom->Error = ResponseCreateRoomError_TooManyRooms;
                                        }
                                        else
                                        {
                                          ResponseCreateRoom->Error = ResponseCreateRoomError_None;
                                          MemoryZeroStruct(Room);

                                          Room->ID = RoomID++;
                                          Room->NameLength = Roomname.Length;
                                          StringAppend(Roomname.Data, RoomName(Room), Roomname.Length);

                                          uptr DefaultMessagePoolSize = Megabytes(1);
                                          Room->MessagePool = MemoryPoolCreate(Win32Alloc(DefaultMessagePoolSize), DefaultMessagePoolSize);

                                          ++RoomCount;
                                        }
                                      }

                                      SendMessage(Client->Socket, RRType_CreateRoom, sizeof(response_create_room)); 
                                    } break;
                                  case RRType_DestroyRoom:
                                    {
                                      request_destroy_room *RequestDestroyRoom = MessageHeaderRequestDestroyRoom(RecvMessageHeader);
                                      room *Room = RoomFind((room *)RoomPool.Memory, RoomCount, RequestDestroyRoom->ID);
                                      if(Room)
                                      {
                                        Win32Free(Room->MessagePool.Memory);
                                        MemoryPoolShift(&RoomPool, RoomNext(Room), Room);

                                        --RoomCount;
                                      }

                                      SendMessage(Client->Socket, RRType_DestroyRoom, sizeof(response_destroy_room)); 
                                    } break;
                                  case RRType_ClientList:
                                    {
                                      request_client_list *RequestClientList = MessageHeaderRequestClientList(RecvMessageHeader);
                                      response_client_list *ResponseClientList = MessageHeaderResponseClientList(SendMessageHeader);

                                      ResponseClientList->ClientInfoCount = 0;

                                      client_info *FirstClientInfo = ClientInfoFirst(ResponseClientList);
                                      client_info *ClientInfo = FirstClientInfo;
                                      credential *Credential = (credential *)CredentialPool.Memory;

                                      for(u32 CredentialIndex = 0;
                                          CredentialIndex < CredentialCount;
                                          ++CredentialIndex)
                                      {
                                        ClientInfo->ID = Credential->ID;
                                        ClientInfo->UsernameLength = Credential->UsernameLength;
                                        StringAppend(CredentialUsername(Credential), ClientInfoUsername(ClientInfo), Credential->UsernameLength); 

                                        ++ResponseClientList->ClientInfoCount;
                                        ClientInfo = ClientInfoNext(ClientInfo);
                                        Credential = CredentialNext(Credential);
                                      }

                                      uptr DataSize = sizeof(response_client_list) + (u8 *)ClientInfo - (u8 *)FirstClientInfo;
                                      SendMessage(Client->Socket, RRType_ClientList, DataSize);
                                    } break;
                                  case RRType_RoomList:
                                    {
                                      response_room_list *ResponseRoomList = MessageHeaderResponseRoomList(SendMessageHeader);
                                      ResponseRoomList->RoomInfoCount = RoomCount;

                                      room_info *FirstRoomInfo = RoomInfoFirst(ResponseRoomList);
                                      room_info *RoomInfo = FirstRoomInfo;
                                      room *Room = (room *)RoomPool.Memory;
                                      for(u32 RoomIndex = 0;
                                          RoomIndex < RoomCount;
                                          ++RoomIndex)
                                      {
                                        RoomInfo->ID = Room->ID;
                                        RoomInfo->NameLength = Room->NameLength;
                                        StringAppend(RoomName(Room), (char *)RoomInfoName(RoomInfo), Room->NameLength);

                                        RoomInfo = RoomInfoNext(RoomInfo);
                                        Room = RoomNext(Room);
                                      }

                                      uptr DataSize = sizeof(response_room_list) + (u8 *)RoomInfo - (u8 *)FirstRoomInfo;
                                      SendMessage(Client->Socket, RRType_RoomList, DataSize);
                                    } break;
                                  case RRType_MessageList:
                                    {
                                      request_message_list *RequestMessageList= MessageHeaderRequestMessageList(RecvMessageHeader);
                                      response_message_list *ResponseMessageList = MessageHeaderResponseMessageList(SendMessageHeader);
                                      MemoryZeroStruct(ResponseMessageList);

                                      uptr ResponseSize = sizeof(response_message_list);
                                      room *MessageRoom = RoomFind((room *)RoomPool.Memory, RoomCount, RequestMessageList->RoomID);
                                      if(MessageRoom)
                                      {
                                        if(RequestMessageList->OnePastLastMessageIndex < MessageRoom->MessageCount)
                                        {
                                          client_message *ClientMessage = (client_message *)(MessageRoom->MessagePool.Memory); 
                                          for(u32 SkipIndex = 0;
                                              SkipIndex < RequestMessageList->OnePastLastMessageIndex;
                                              ++SkipIndex)
                                          {
                                            ClientMessage = ClientMessageNext(ClientMessage); 
                                          }

                                          uptr DataOffset = MemoryByteOffset(SendBuffer, ResponseMessageList + 1);
                                          uptr AvailableSize = Message_SizeMax - DataOffset;

                                          uptr MessageInfoSizeTotal = 0;
                                          uptr StringSizeTotal = 0;

                                          u32 MessageCountMax = (u32)(MessageRoom->MessageCount - RequestMessageList->OnePastLastMessageIndex);
                                          u32 MessageCount = 0;
                                          {
                                            client_message *CountMessage = ClientMessage;
                                            while(MessageCount < MessageCountMax)
                                            {
                                              uptr MessageInfoSize = sizeof(message_info);
                                              uptr StringSize = CountMessage->Length;

                                              uptr SizeTotal = MessageInfoSizeTotal + MessageInfoSize + StringSizeTotal + StringSize;
                                              if(SizeTotal > AvailableSize)
                                              {
                                                break;
                                              }
                                              else
                                              {
                                                MessageInfoSizeTotal += MessageInfoSize;
                                                StringSizeTotal += StringSize;

                                                ++MessageCount;
                                                CountMessage = ClientMessageNext(CountMessage);
                                              }
                                            }
                                          }
                                          
                                          ResponseMessageList->MessageInfoCount = MessageCount;
                                          ResponseMessageList->MessageInfoOffset = (u64)DataOffset;

                                          message_info *MessageInfo = (message_info *)(SendBuffer + ResponseMessageList->MessageInfoOffset);
                                          uptr StringOffset = (u64)(DataOffset + MessageInfoSizeTotal);

                                          for(u32 MessageIndex = 0;
                                              MessageIndex < MessageCount;
                                              ++MessageIndex)
                                          {
                                            MessageInfo->StringOffset = (u64)StringOffset;
                                            MessageInfo->StringLength = ClientMessage->Length;
                                            MessageInfo->ID = ClientMessage->ID;

                                            MemoryCopy(ClientMessageMessage(ClientMessage), SendBuffer + StringOffset, ClientMessage->Length);

                                            StringOffset += ClientMessage->Length; 
                                            ++MessageInfo;
                                            ClientMessage = ClientMessageNext(ClientMessage); 
                                          }

                                          ResponseSize = StringOffset;
                                        }
                                      }

                                      SendMessage(Client->Socket, RRType_MessageList, ResponseSize);
                                    } break;
                                  case RRType_SendMessage:
                                    {
                                      request_send_message *RequestSendMessage= MessageHeaderRequestSendMessage(RecvMessageHeader);

                                      room *MessageRoom = RoomFind((room *)RoomPool.Memory, RoomCount, RequestSendMessage->RoomID);
                                      if(MessageRoom)
                                      {
                                        memory_pool *MessagePool = &MessageRoom->MessagePool;

                                        uptr RequiredSize = sizeof(client_message) + RequestSendMessage->MessageLength;
                                        client_message *ClientMessage = 0;

                                        if(MemoryPoolSizeRemaining(MessagePool) >= RequiredSize)
                                        {
                                          ClientMessage = (client_message *)MemoryPoolPushSize(MessagePool, RequiredSize); 
                                        }
                                        else
                                        {
                                          uptr SizeNew = NextPow2(MessagePool->SizeTotal + RequiredSize);
                                          void *MemoryNew = Win32Alloc(SizeNew);

                                          if(MemoryNew)
                                          {
                                            Win32Free(MemoryPoolCopyMove(MessagePool, MemoryNew, SizeNew));
                                            ClientMessage = (client_message *)MemoryPoolPushSize(MessagePool, RequiredSize); 
                                          }
                                        }

                                        if(ClientMessage)
                                        {
                                          credential *Credential = (credential *)((u8 *)CredentialPool.Memory + Client->CredentialOffset);
                                          ClientMessage->ID = Credential->ID;
                                          ClientMessage->Length = RequestSendMessage->MessageLength;
                                          MemoryCopy(RequestSendMessageMessage(RequestSendMessage), ClientMessageMessage(ClientMessage), RequestSendMessage->MessageLength);

                                          ++MessageRoom->MessageCount;
                                        }
                                      }

                                      SendMessage(Client->Socket, RRType_SendMessage, sizeof(request_send_message));
                                    } break;
                                    InvalidDefaultCase;
                                }
                              }
                            }
                          } break;
                      }
                    }
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
                }

                // save
                {
                  HANDLE FileHandle = CreateFileA(SaveFilename, GENERIC_WRITE, 0, 0, OPEN_ALWAYS, 0, 0);
                  if(FileHandle != INVALID_HANDLE_VALUE)
                  {
                    // TODO maybe check?
                    auto WriteFileData = [&FileHandle](u64 Offset, void *Src, u32 SrcSize)
                    {
                      LONG OffsetLow = (LONG)Offset;
                      LONG OffsetHigh = (LONG)(Offset >> 32);
                      SetFilePointer(FileHandle, OffsetLow, &OffsetHigh, 0);

                      DWORD Ignored;
                      WriteFile(FileHandle, Src, SrcSize, &Ignored, 0);
                    };

                    /*
                       struct file_header
                       {
                       };

                       struct file_room
                       {
                       u32 NameLength;
                       u32 MessageCount;

                       u64 NameOffset;
                       u64 MessageOffset;
                       u64 MessageSize;
                       };

                     */
                     file_header FileHeader = {};
                     FileHeader.MagicValue = FileHeaderMagicValue;

                     FileHeader.CredentialOffset = sizeof(file_header);
                     FileHeader.CredentialCount = CredentialCount;
                     FileHeader.CredentialSize = CredentialPool.SizeUsed;
                     WriteFileData(sizeof(file_header), CredentialPool.Memory, (u32)CredentialPool.SizeUsed);

                     FileHeader.RoomOffset = FileHeader.CredentialOffset + FileHeader.CredentialSize;
                     FileHeader.RoomCount = RoomCount;
                     {
                       room *Room = (room *)RoomPool.Memory;
                       for(u32 RoomIndex = 0;
                           RoomIndex < RoomCount;
                           ++RoomIndex)
                       {
                         FileHeader.RoomnameSize += Room->NameLength;
                         Room = RoomNext(Room);
                       }
                     }
                     
                     u64 RoomOffset = FileHeader.RoomOffset;
                     u64 NameOffset = RoomOffset + sizeof(file_room) * RoomCount;
                     u64 MessageOffset = NameOffset + FileHeader.RoomnameSize;
                     {
                       room *Room = (room *)RoomPool.Memory;
                       for(u32 RoomIndex = 0;
                           RoomIndex < RoomCount;
                           ++RoomIndex)
                       {
                         file_room FileRoom;
                         FileRoom.NameLength = Room->NameLength;
                         FileRoom.MessageCount = Room->MessageCount;
                         FileRoom.NameOffset = NameOffset;
                         FileRoom.MessageOffset = MessageOffset;
                         FileRoom.MessageSize = Room->MessagePool.SizeUsed;

                         WriteFileData(RoomOffset, &FileRoom, sizeof(FileRoom));
                         WriteFileData(NameOffset, RoomName(Room), Room->NameLength);
                         WriteFileData(MessageOffset, Room->MessagePool.Memory, (u32)Room->MessagePool.SizeUsed);

                         RoomOffset += sizeof(file_room);
                         NameOffset += Room->NameLength;
                         MessageOffset += Room->MessagePool.SizeUsed;

                         Room = RoomNext(Room);
                       }
                     }
                     WriteFileData(0, &FileHeader, sizeof(FileHeader));
                  }
                }
                closesocket(ListenSocket);
                OutputTextCharArray("[SERVER]: Shutting down!");
              }
            }
          }
        }
      }
    }

    WSACleanup();
  }

  return Result;
}


