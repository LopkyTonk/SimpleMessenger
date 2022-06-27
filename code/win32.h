#pragma once

#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#include "winsock2.h"
#include "ws2tcpip.h"

void *
Win32Alloc(uptr Size)
{
  void *Result = VirtualAlloc(0, Size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE); 
  return Result;
}

void
Win32Free(void *Free)
{
  VirtualFree(Free, 0, MEM_RELEASE);
}

struct socket_poll_result
{
  u32 State;
  u16 Events;
};

socket_poll_result
SocketPoll(SOCKET Socket, u16 CheckEvents, s32 Timeout)
{
  pollfd PollInfo = {};
  PollInfo.fd = Socket;
  PollInfo.events = CheckEvents;

  u32 PollState = WSAPoll(&PollInfo, 1, Timeout);

  socket_poll_result Result;
  Result.State = PollState;
  Result.Events = PollInfo.revents;
  
  return Result;
}

socket_poll_result
SocketPoll_(SOCKET Socket, u16 CheckEvents, s32 Timeout)
{
  socket_poll_result Result = SocketPoll(Socket, CheckEvents, Timeout);

  b32 HasErrors = Result.Events ^ CheckEvents;
  if(HasErrors)
  {
    Result.Events = 0;
  }

  return Result;
}

socket_poll_result
SocketPollDisconnected(SOCKET Socket, s32 Timeout = 0)
{
  socket_poll_result Result = SocketPoll(Socket, 0, Timeout);
  return Result;
}

socket_poll_result
SocketPollRead(SOCKET Socket, s32 Timeout = 0)
{
  socket_poll_result Result = SocketPoll_(Socket, POLLRDNORM, Timeout);
  return Result;
}
socket_poll_result
SocketPollReadWithErrors(SOCKET Socket, s32 Timeout = 0)
{
  socket_poll_result Result = SocketPoll(Socket, POLLRDNORM, Timeout);
  return Result;
}

socket_poll_result
SocketPollWrite(SOCKET Socket, s32 Timeout = 0)
{
  socket_poll_result Result = SocketPoll_(Socket, POLLWRNORM, Timeout);
  return Result;
}
