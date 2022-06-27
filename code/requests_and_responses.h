#pragma once

#define Message_SizeMax 1'048'576 

// RR =  request response
enum rr_type
{
  RRType_CreateAccount,
  RRType_Login,
  RRType_Logout,
  RRType_CreateRoom,
  RRType_DestroyRoom,
  RRType_ClientList,
  RRType_RoomList,
  RRType_MessageList,
  RRType_SendMessage,
  RRType_Count,

  RRType_First = 0,
  RRType_Last = RRType_Count - 1
};

///
/// Request logins
///

/*
* Request create_account 
* Memory layout:
* request_create_account CreateAccount  ...
* char Username[CreateAccount.UsernameLength]      ...
* char Password[CreateAccount.PasswordLength]
*/
struct request_create_account
{
#define UsernameLengthMax 256
#define PasswordLengthMax 256
  u32 UsernameLength;
  u32 PasswordLength;
};
char *
RequestCreateAccountUsername(request_create_account *CreateAccount)
{
  uptr ByteOffset = sizeof(request_create_account);
  char *Result = (char *)((u8 *)CreateAccount + ByteOffset);
  return Result;
}
char *
RequestCreateAccountPassword(request_create_account *CreateAccount)
{
  uptr ByteOffset = sizeof(request_create_account) + CreateAccount->UsernameLength;
  char *Result = (char *)((u8 *)CreateAccount + ByteOffset);
  return Result;
}

//
/*
* Request login 
* Memory layout:
* request_login Login  ...
* char Username[Login.UsernameLength]      ...
* char Password[Login.PasswordLength]
*/
struct request_login
{
  u32 UsernameLength;
  u32 PasswordLength;
};
char *
RequestLoginUsername(request_login *Login)
{
  uptr ByteOffset = sizeof(request_login);
  char *Result = (char *)((u8 *)Login + ByteOffset);
  return Result;
}
char *
RequestLoginPassword(request_login *Login)
{
  uptr ByteOffset = sizeof(request_login) + Login->UsernameLength;
  char *Result = (char *)((u8 *)Login + ByteOffset);
  return Result;
}
//

/*
* Request login logout 
* Memory layout:
* request_logout Logout
*/
struct request_logout{};
//

/*
* Request create room 
* Memory layout:
* request_create_room CreateAccount  ...
* char Username[CreateAccount.UsernameLength]      ...
* char Password[CreateAccount.PasswordLength]
*/
struct request_create_room
{
  u32 NameLength;
};
char *
RequestCreateRoomName(request_create_room *RequestCreateRoom)
{
  uptr ByteOffset = sizeof(request_create_room);
  char *Result = (char *)((u8 *)RequestCreateRoom + ByteOffset);
  return Result;
}
//

/*
* Memory layout:
* request_client_list DestroyRoom 
*/
struct request_destroy_room
{
  u32 ID;
};
//

/*
* Memory layout:
* request_client_list ClientList 
*/
struct request_client_list
{
};
//

/*
* Memory layout:
* request_room_list RoomList 
*/
struct request_room_list
{
};
//

/*
* Memory layout:
* request_room_list RoomList 
*/
struct request_message_list
{
  u32 RoomID;
  u64 OnePastLastMessageIndex;
};
//
/*
* Memory layout:
* request_send_message SendMessage  ...
* char Message[SendMessage.MessageLength]
*/
struct request_send_message
{
  u32 RoomID;
  u32 MessageLength;
};
char *
RequestSendMessageMessage(request_send_message *SendMessage)
{
  char *Result = (char *)(SendMessage + 1);
  return Result;
}

///
/// Request logins end
///


///
/// Responses
///

/*
* Memory layout:
* response_create_account CreateAccount 
*/

enum response_create_account_error
{
  ResponseCreateAccountError_None,
  ResponseCreateAccountError_UsernameExists,
  ResponseCreateAccountError_TooManyAccounts
};
struct response_create_account
{
  response_create_account_error Error;
};
//

/*
* Response login 
* Memory layout:
* response_login Login 
*/
enum response_login_error
{
  ResponseLoginError_None,
  ResponseLoginError_InvalidUsernameOrPassword,
};
struct response_login
{
  response_login_error Error;
};
//


/*
* Response logout 
* Memory layout:
* response_logout Logout 
*/
struct response_logout{};
//

/*
* Memory layout:
* response_destroy_room DestroyAccount 
*/
struct response_destroy_room
{
};
//

/*
* Memory layout:
* response_create_room CreateAccount 
*/

enum response_create_room_error
{
  ResponseCreateRoomError_None,
  ResponseCreateRoomError_RoomnameExists,
  ResponseCreateRoomError_TooManyRooms
};
struct response_create_room
{
  response_create_room_error Error;
};
//

/*
* Response client list
* Memory layout:
* response_client_list ClientList ...
* { 
*   client_info ClientInfo ...
*   char ClientUsername[ClientInfo.UsernameLength] ...
* } ClientList.ClientInfoCount 
*/
struct 
response_client_list
{
  u32 ClientInfoCount;
};
struct
client_info
{
  u32 ID;
  u32 UsernameLength;
};
client_info *
ClientInfoFirst(response_client_list *ResponseClientList)
{
  client_info *Result = (client_info *)(ResponseClientList + 1);
  return Result;
}
client_info *
ClientInfoNext(client_info *ClientInfo)
{
  uptr ByteOffset = sizeof(client_info) + ClientInfo->UsernameLength;
  client_info *Result = (client_info *)((u8 *)ClientInfo + ByteOffset);

  return Result;
}

char *
ClientInfoUsername(client_info *Client)
{
  char *Result = (char *)(Client + 1);
  return Result;
}
//

/*
* Response room list
* Memory layout:
* response_room_list RoomList ...
* { 
*   room_info RoomInfo ...
*   char RoomInfoName[RoomInfo.NameLength] ...
* } RoomList.RoomInfoCount 
*/
struct response_room_list
{
  u32 RoomInfoCount;
};
struct room_info
{
  u32 ID;
  u32 NameLength;
};

room_info *
RoomInfoFirst(response_room_list *ResponseRoomList)
{
  room_info *Result = (room_info *)(ResponseRoomList + 1);
  return Result;
}
room_info *
RoomInfoNext(room_info *RoomInfo)
{
  uptr AdvanceBytes = sizeof(room_info) + RoomInfo->NameLength;
  room_info *Result = (room_info *)((u8 *)RoomInfo + AdvanceBytes);
  return Result;
}
char *
RoomInfoName(room_info *RoomInfo)
{
  char *Result = (char *)(RoomInfo + 1);
  return Result;
}

/*
* Response message list
* Memory layout:
* response_message_list MessageList ...
* { 
*   message_info MessageInfo ...
*   char Message[MessageInfo.Length]
* } MessageList.MessageInfoCount 
*/
struct response_message_list
{
  u64 MessageInfoOffset;
  u64 MessageInfoCount;
};
struct message_info
{
  u64 StringOffset;
  u32 StringLength;
  u32 ID;
};

/*
* Response send message
* Memory layout:
* response_send_message SendMessage
*/
struct response_send_message {};

///

// 
// Message header
// 

#define MessageHeaderMagicValue 0x01234567
struct message_header
{
  u32 MagicValue;
  rr_type RRType;
};
#define MessageHeaderIsValid(MessageHeader)(MessageHeader->MagicValue == MessageHeaderMagicValue)

#define MakeRRFunctions(UppercaseName, LowercaseName) \
response_##LowercaseName## * \
MessageHeaderResponse##UppercaseName##(message_header *MessageHeader) \
{ \
  return (response_##LowercaseName## *)(MessageHeader + 1); \
} \
request_##LowercaseName## * \
MessageHeaderRequest##UppercaseName##(message_header *MessageHeader) \
{ \
  return (request_##LowercaseName## *)(MessageHeader + 1); \
}

MakeRRFunctions(CreateAccount, create_account);
MakeRRFunctions(Login, login);
MakeRRFunctions(Logout, logout);
MakeRRFunctions(CreateRoom, create_room);
MakeRRFunctions(DestroyRoom, destroy_room);
MakeRRFunctions(ClientList, client_list);
MakeRRFunctions(RoomList, room_list);
MakeRRFunctions(MessageList, message_list);
MakeRRFunctions(SendMessage, send_message);
