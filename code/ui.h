#pragma once

enum ui_widget_type : u32
{
  UIWidgetType_InputBox,
  UIWidgetType_ButtonBox,
  UIWidgetType_TextBox,
  UIWidgetType_Room,
  UIWidgetType_Message,

  UIWidgetType_Count,
  UIWidgetType_Last = UIWidgetType_Count - 1,
  UIWidgetType_First = 0,
};

enum ui_input_filter
{
  UIInputFilter_NumbersOrLetters = 1 << 0,
  UIInputFilter_FirstNumberOrLetter = 1 << 1,
};
struct ui_input_box
{
  ui_input_filter Filter;
  struct ui_char_buffer *CharBuffer;
  b32 CanBeRead;
};

struct ui_button_box
{
  string Text;
  u32 TextColor;
};

struct ui_text_box
{
  string Text;
  u32 TextColor;
};

struct ui_room
{
  string Text;
};
struct ui_message
{
  string Username;
  string Message;
};

enum ui_widget_event : u32
{
  UIWidgetEvent_Hovered,
  UIWidgetEvent_Pressed,
  UIWidgetEvent_Count,

  UIWidgetEvent_First = 0,
  UIWidgetEvent_OnePastLast = UIWidgetEvent_Count
};
#define UIWidgetEventBit(Event)(1 << Event)

enum ui_widget_state : u32
{
  UIWidgetState_Active,
  UIWidgetState_Count,

  UIWidgetState_First = 0,
  UIWidgetState_OnePastLast = UIWidgetState_Count
};
#define UIWidgetStateBit(State)(1 << State)

enum ui_size_type
{
  UISizeType_Absolute,
  UISizeType_Relative,
  UISizeType_Fill
};

struct ui_widget
{
  ui_widget *Prev;
  ui_widget *Next;

  ui_widget *TypePrev;
  ui_widget *TypeNext;

  ui_widget_type Type;

  u32 FillColor;
  r32 OutlineThickness;
  u32 OutlineColor;

  ui_size_type WidthMinType;
  ui_size_type HeightMinType;
  r32 WidthMin;
  r32 HeightMin;

  rect VisibleRect;

  u32 Event_;
  u32 State_;
};

enum ui_form_layout
{
  UIFormLayout_TopToBottom,
  UIFormLayout_BottomToTop,
  UIFormLayout_LeftToRight,
};
struct ui_range_widget
{
  ui_widget *First;
  ui_widget *Last;
};

struct ui_window;
struct ui_form
{
  union
  {
    ui_form *NextFree;
    ui_form *Next;
  };
  
  ui_window *Window;
  ui_form_layout Layout;
  v2 NormalizedPosition; 
  v2 NormalizedSize;

  u32 BackgroundColor;
  s32 ScrollAmmount;
  s32 ScrollAmmountMax;
  
  v2 Position;
  v2 VisibleSize;
  rect VisibleRect;

  ui_range_widget Range;
  ui_range_widget TypeRange[UIWidgetType_Count];
};

struct ui_char_buffer
{ 
  union
  {
    ui_char_buffer *NextFree;
    u32 LengthMax;
  };

  string String;
};

struct ui_window
{
  memory_pool MemoryPool;
  bitmap Bitmap_;
  bitmap Bitmap;
  u32 ClearColor;

  ui_form *FirstFreeForm;
  ui_widget *FirstFreeWidget[UIWidgetType_Count];

  ui_char_buffer EmptyCharBuffer;
  ui_char_buffer *FirstFreeCharBuffer;
  
  b32 MouseDown;
  s32 ScrollOffset;
  v2 CursorPosition;

  ui_form *FirstForm;
  ui_form *LastForm;

  ui_widget *WidgetWithEvent[UIWidgetEvent_Count];
  ui_widget *WidgetWithState[UIWidgetState_Count];
};

ui_window 
UIWindow(void *Memory, uptr MemorySize, u32 Width, u32 Height, u32 ClearColor)
{
  ui_window Result = {};

  u32 BitmapSize = 4 * Width * Height;
  Result.MemoryPool = MemoryPoolCreate((u8 *)Memory + BitmapSize, MemorySize - BitmapSize);
   
  Result.Bitmap_ = Bitmap((u32 *)(Memory), Width, Height);
  Result.ClearColor = ClearColor;

  return Result;
}

ui_form * 
UIPushForm(ui_window *Window, ui_form_layout Layout, v2 NormalizedPosition, v2 NormalizedSize)
{
  Assert(NormalizedPosition.X >= 0.0f && NormalizedPosition.X <= 1.0f);
  Assert(NormalizedPosition.Y >= 0.0f && NormalizedPosition.Y <= 1.0f);
  v2 CheckCorner = NormalizedPosition + NormalizedSize;
  Assert(CheckCorner.X >= 0.0f && CheckCorner.X <= 1.0f);
  Assert(CheckCorner.Y >= 0.0f && CheckCorner.Y <= 1.0f);

  ui_form *Result = 0;
  if(Window->FirstFreeForm)
  {
    Result = Window->FirstFreeForm;
    Window->FirstFreeForm = Window->FirstFreeForm->NextFree;
  }
  else
  {
    Result = MemoryPoolTryPushStruct(&Window->MemoryPool, ui_form);
  }

  if(Result)
  {
    MemoryZeroStruct(Result);

    Result->Window = Window;
    Result->Next = {};
    Result->Layout = Layout;
    Result->NormalizedPosition = NormalizedPosition;
    Result->NormalizedSize = NormalizedSize;
    Result->BackgroundColor = Window->ClearColor;

    if(Window->LastForm)
    {
      Window->LastForm->Next = Result;
      Window->LastForm = Result;
    }
    else
    {
      Window->FirstForm = Window->LastForm = Result;
    }
  }
  return Result;
}

  void
UIResetWidgetEvent(ui_window *Window, ui_widget *Widget, ui_widget_event Event)
{
  u32 EventBit = UIWidgetEventBit(Event);
  Assert((Widget->Event_ & EventBit) == EventBit);
  Widget->Event_ &= ~EventBit;

  ui_widget **WidgetWithEvent = Window->WidgetWithEvent + Event;
  Assert(*WidgetWithEvent == Widget);
  *WidgetWithEvent = 0;
}
  void
UISetWidgetEvent(ui_window *Window, ui_widget *Widget, ui_widget_event Event)
{
  u32 EventBit = UIWidgetEventBit(Event);
  Assert((Widget->Event_ & EventBit) == 0);
  Widget->Event_ |= EventBit;

  ui_widget **WidgetWithEvent = Window->WidgetWithEvent + Event;
  Assert(*WidgetWithEvent == 0);
  *WidgetWithEvent = Widget;
}

  void
UIResetWidgetState(ui_window *Window, ui_widget *Widget, ui_widget_state State)
{
  u32 StateBit = UIWidgetStateBit(State);
  Assert((Widget->State_ & StateBit) == StateBit);
  Widget->State_ &= ~StateBit;

  ui_widget **WidgetWithState = Window->WidgetWithState + State;
  Assert(*WidgetWithState == Widget);
  *WidgetWithState = 0;
}
  void
UISetWidgetState(ui_window *Window, ui_widget *Widget, ui_widget_state State)
{
  u32 StateBit = UIWidgetStateBit(State);
  Assert((Widget->State_ & StateBit) == 0);
  Widget->State_ |= StateBit;

  ui_widget **WidgetWithState = Window->WidgetWithState + State;
  Assert(*WidgetWithState == 0);
  *WidgetWithState = Widget;
}
#if 0
void
UIWindowSetNextActiveWidget(ui_window *Window)
{
  ui_id ActiveWidgetID = Window->ActiveWidgetID;
  if(UIIDValid(ActiveWidgetID))
  {
    UIWindowSetWidgetEvent(Window, ActiveWidgetID, UIWidgetEvent_Deactivated);

    for(ui_id NextWidget = ActiveWidget->Next;
        NextWidget;)
    {
      switch(NextWidget->Type)
      {
        case UIWidgetType_InputBox:
        case UIWidgetType_ButtonBox:
          {
            UIWindowSetWidgetEvent(Window, NextWidget, UIWidgetEvent_Activated);
          } break;
        case UIWidgetType_TextBox:
          {
          } break;
          InvalidDefaultCase;
      }
      if(Window->ActiveWidget)
      {
        break;
      }
      else
      {
        NextWidget = NextWidget->Next;
      }
    }
  }
}
#endif 

void
UIFormSetBackgroundColor(ui_form *Form, u32 BackgroundColor)
{
  Form->BackgroundColor = BackgroundColor;
}

ui_widget *
UIPushFirstWidget_(ui_form *Form, ui_widget_type Type, uptr TypeSize)
{
  ui_window *Window = Form->Window;

  ui_widget *Result;
  if(Window->FirstFreeWidget[Type])
  {
    Result = Window->FirstFreeWidget[Type];
    Window->FirstFreeWidget[Type] = Window->FirstFreeWidget[Type]->TypeNext;
    MemoryZero(Result, sizeof(ui_widget) + TypeSize);
  }
  else
  {
    Result = (ui_widget *)MemoryPoolTryPushSize(&Window->MemoryPool, sizeof(ui_widget) + TypeSize); 
  }

  if(Result)
  {
    MemoryZero(Result, sizeof(ui_widget) + TypeSize);
    Result->Type = Type;
    Result->FillColor = Form->BackgroundColor;

    ui_range_widget *Range = &Form->Range;
    ui_range_widget *TypeRange = Form->TypeRange + Type;

    if(!TypeRange->First)
    {
      TypeRange->First = TypeRange->Last = Result;
      Result->TypePrev = Result->TypeNext = 0;

      if(Range->Last)
      {
        Range->Last->Next = Result;
        Result->Prev = Range->Last;
        Range->Last = Result;

        Result->Next = 0;
      }
      else
      {
        Range->First = Range->Last = Result;
        Result->Prev = Result->Next = 0;
      }
    }
    else
    {
      ui_widget *Replace = TypeRange->First;
      TypeRange->First = Result;

      Result->TypePrev = 0;
      Result->TypeNext = Replace;

      if(!Replace->Prev)
      {
        Range->First = Result; 
      }
      else
      {
        Replace->Prev->Next = Result;
      }
      
      Result->Prev = Replace->Prev;
      Replace->Prev = Result;
      Result->Next = Replace;
    }
  }
  return Result;
}

ui_widget *
UIPushWidget_(ui_form *Form, ui_widget_type Type, uptr TypeSize)
{
  ui_window *Window = Form->Window;

  ui_widget *Result;
  if(Window->FirstFreeWidget[Type])
  {
    Result = Window->FirstFreeWidget[Type];
    Window->FirstFreeWidget[Type] = Window->FirstFreeWidget[Type]->TypeNext;
    MemoryZero(Result, sizeof(ui_widget) + TypeSize);
  }
  else
  {
    Result = (ui_widget *)MemoryPoolTryPushSize(&Window->MemoryPool, sizeof(ui_widget) + TypeSize); 
  }

  if(Result)
  {
    MemoryZero(Result, sizeof(ui_widget) + TypeSize);
    Result->Type = Type;
    Result->FillColor = Form->BackgroundColor;
    
    Result->Next = 0;
    ui_range_widget *Range = &Form->Range;
    if(Range->Last)
    {
      Range->Last->Next = Result;
      Result->Prev = Range->Last;

      Range->Last = Result;
    }
    else
    {
      Result->Prev = 0;
      Range->First = Range->Last = Result;
    }

    Result->TypeNext = 0;
    ui_range_widget *TypeRange = Form->TypeRange + Type;
    if(TypeRange->Last)
    {
      TypeRange->Last->TypeNext = Result;
      Result->TypePrev = TypeRange->Last;

      TypeRange->Last = Result;
    }
    else
    {
      Result->TypePrev = 0;
      TypeRange->First = TypeRange->Last = Result;
    }
  }
  return Result;
}

#define UI_WIDGET_MAKE_FUNCTIONS_(LowercaseName, UppercaseName, Empty) \
uptr \
UI##UppercaseName##Size() \
{ \
  return sizeof(ui_widget) + sizeof(ui_##LowercaseName##); \
} \
ui_widget * \
UIGetWidget(ui_##LowercaseName## *Widget) \
{ \
  return (ui_widget *)((u8 *)Widget - sizeof(ui_widget)); \
} \
ui_##LowercaseName## * \
UIGet##UppercaseName##(ui_widget *Widget) \
{ \
  return (ui_##LowercaseName## *)((u8 *)Widget + sizeof(ui_widget)); \
} \
\
void UIInitialize##UppercaseName##_(ui_form *Form, ui_##LowercaseName## *Widget); \
ui_##LowercaseName## *\
UIPush##UppercaseName##(ui_form *Form) \
{ \
  ui_##LowercaseName## *Result = 0; \
  ui_widget *Widget = UIPushWidget_(Form, UIWidgetType_##UppercaseName##, sizeof(ui_##LowercaseName##)); \
  if(Widget)\
  { \
    Result = UIGet##UppercaseName##(Widget); \
    UIInitialize##UppercaseName##_(Form, Result);\
  } \
  return Result; \
} \
ui_##LowercaseName## *\
UIPushFirst##UppercaseName##(ui_form *Form) \
{ \
  ui_##LowercaseName## *Result = 0; \
  ui_widget *Widget = UIPushFirstWidget_(Form, UIWidgetType_##UppercaseName##, sizeof(ui_##LowercaseName##)); \
  if(Widget)\
  { \
    Result = UIGet##UppercaseName##(Widget); \
    UIInitialize##UppercaseName##_(Form, Result);\
  } \
  return Result; \
} \
b32 \
UIIsEvent(ui_##LowercaseName## *WidgetType, ui_widget_event Event) \
{ \
  ui_widget *Widget = UIGetWidget(WidgetType); \
  u32 EventBit = UIWidgetEventBit(Event); \
  return (Widget->Event_ & EventBit) == EventBit; \
} \
b32 \
UIIsState(ui_##LowercaseName## *WidgetType, ui_widget_state State) \
{ \
  ui_widget *Widget = UIGetWidget(WidgetType); \
  u32 StateBit = UIWidgetStateBit(State); \
  return (Widget->State_ & StateBit) == StateBit; \
} \
void \
UISetFillColor(ui_##LowercaseName## *WidgetType, u32 FillColor) \
{ \
  ui_widget *Widget = UIGetWidget(WidgetType); \
  Widget->FillColor = FillColor; \
} \
void \
UISetWidthMin(ui_##LowercaseName## *WidgetType, ui_size_type Type, r32 WidthMin) \
{ \
  ui_widget *Widget = UIGetWidget(WidgetType); \
  Widget->WidthMinType = Type; \
  Widget->WidthMin = WidthMin; \
} \
void \
UISetHeightMin(ui_##LowercaseName## *WidgetType, ui_size_type Type, r32 HeightMin) \
{ \
  ui_widget *Widget = UIGetWidget(WidgetType); \
  Widget->HeightMinType = Type; \
  Widget->HeightMin = HeightMin; \
} \
void \
UISetOutline(ui_##LowercaseName## *WidgetType, r32 Thickness, u32 Color) \
{ \
  ui_widget *Widget = UIGetWidget(WidgetType); \
  Widget->OutlineThickness = Thickness; \
  Widget->OutlineColor = Color; \
} 
#define UI_WIDGET_MAKE_FUNCTIONS(LowercaseName, UppercaseName) UI_WIDGET_MAKE_FUNCTIONS_(LowercaseName, UppercaseName)

UI_WIDGET_MAKE_FUNCTIONS(input_box, InputBox);
void
UIInitializeInputBox_(ui_form *Form, ui_input_box *InputBox)
{
  ui_window *Window = Form->Window;

  u32 CharBufferLengthMax = 256;
  ui_char_buffer *CharBuffer;
  if(Window->FirstFreeCharBuffer)
  {
    CharBuffer = Window->FirstFreeCharBuffer;
    Window->FirstFreeCharBuffer = CharBuffer->NextFree;

    CharBuffer->LengthMax = CharBufferLengthMax;
    CharBuffer->String.Length = 0;
  }
  else
  {
    CharBuffer = (ui_char_buffer *)MemoryPoolTryPushSize(&Window->MemoryPool, sizeof(ui_char_buffer) + CharBufferLengthMax);

    if(CharBuffer)
    {
      CharBuffer->LengthMax = CharBufferLengthMax;
      CharBuffer->String = StringDataLength((char *)(CharBuffer + 1), 0);
    }
    else
    {
      CharBuffer = &Window->EmptyCharBuffer;
    }
  }

  InputBox->CharBuffer = CharBuffer;

  UISetOutline(InputBox, 1.0f, 0xff000000);
  UISetFillColor(InputBox, 0xffaaaaaa);
}

void
UIPushKeycode(ui_input_box *InputBox, char Keycode)
{
  ui_char_buffer *CharBuffer = InputBox->CharBuffer;
  string *String = &CharBuffer->String;
  
  if(!InputBox->CanBeRead)
  {
    if(String->Length < CharBuffer->LengthMax)
    {
      b32 Valid = true;
      if(InputBox->Filter & UIInputFilter_NumbersOrLetters && Valid)
      {
        Valid = (Keycode >= '0' && Keycode <= '9') || (Keycode >= 'A' && Keycode <= 'Z') || (Keycode >= 'a' && Keycode <= 'z');
      }
      if(InputBox->Filter & UIInputFilter_FirstNumberOrLetter && Valid)
      {
        Valid = String->Length > 0 || Keycode != ' '; 
      }

      if(Valid)
      {
        String->Data[String->Length++] = Keycode;
      }
    }
  }
}
void
UIPopKeycode(ui_input_box *InputBox)
{
  string *String = &InputBox->CharBuffer->String;

  InputBox->CanBeRead = false;
  if(String->Length)
  {
    String->Length--;
  }
}

UI_WIDGET_MAKE_FUNCTIONS(text_box, TextBox);
void
UIInitializeTextBox_(ui_form *Form, ui_text_box *TextBox)
{
}

UI_WIDGET_MAKE_FUNCTIONS(button_box, ButtonBox);
void
UIInitializeButtonBox_(ui_form *Form, ui_button_box *ButtonBox)
{
  UISetOutline(ButtonBox, 1.0f, 0xff000000);
}

UI_WIDGET_MAKE_FUNCTIONS(room, Room);
void
UIInitializeRoom_(ui_form *Form, ui_room *Room)
{
}

UI_WIDGET_MAKE_FUNCTIONS(message, Message);
void
UIInitializeMessage_(ui_form *Form, ui_message *Message)
{
}

#if 0
  void 
UIInputBoxSetLabel(ui_widget *Widget, string Label)
{
  Assert(Widget->Type == UIWidgetType_InputBox);
  ui_input_box *InputBox = &Widget->InputBox;

  InputBox->Label = Label;
}
#endif

void
UINextFrame(ui_window *Window, resource_font *Font)
{
  for(ui_widget_event ClearEvent = UIWidgetEvent_First;
      ClearEvent < UIWidgetEvent_OnePastLast;
      ClearEvent = (ui_widget_event)(ClearEvent + 1))
  {
    ui_widget *ClearWidget = Window->WidgetWithEvent[ClearEvent];
    if(ClearWidget)
    {
      UIResetWidgetEvent(Window, ClearWidget, ClearEvent);
    }
  }

  RenderFillRect(RectMinMax(0, 0, 10000, 10000), Window->ClearColor, &Window->Bitmap); 
  
  u32 WindowWidth = Window->Bitmap.Width;
  u32 WindowHeight = Window->Bitmap.Height;
  v2 WindowSize = V2U32(WindowWidth, WindowHeight);

  for(ui_form *Form = Window->FirstForm;
      Form;)
  {
    v2 FormSize = Hadamard(Form->NormalizedSize, WindowSize);
    v2 FormPosition = Hadamard(Form->NormalizedPosition, WindowSize); 

    u32 FormX = (u32)Round(FormPosition.X);
    u32 FormWidth = (u32)Round(FormSize.X);
    if(FormX + FormWidth > WindowWidth)
    {
      FormWidth = WindowWidth - FormX;
    }

    u32 FormY = (u32)Round(FormPosition.Y);
    u32 FormHeight = (u32)Round(FormSize.Y);
    if(FormY + FormHeight > WindowHeight)
    {
      FormHeight = WindowHeight - FormY;
    }
    bitmap RenderBitmap = BitmapView(&Window->Bitmap, FormWidth, FormHeight, FormX, FormY);

    RenderClear(Form->BackgroundColor, &RenderBitmap); 

    v2 ScrollOffset = V2(0.0f, 0.0f);
    switch(Form->Layout)
    {
      case UIFormLayout_TopToBottom:
      case UIFormLayout_BottomToTop:
        {
          r32 VisibleHeight = Form->VisibleSize.Y;
          r32 MaxHeight = FormSize.Y;
          
          s32 PrevScrollAmmountMax = Form->ScrollAmmountMax;
          Form->ScrollAmmountMax = (s32)Max(0, VisibleHeight - MaxHeight);
          s32 ScrollAmmounMaxDelta = Form->ScrollAmmountMax - PrevScrollAmmountMax;

          if(Form->ScrollAmmountMax == 0)
          {
            Form->ScrollAmmount = 0;
          }
          else
          {
            Form->ScrollAmmount = Clamp(Form->ScrollAmmount + ScrollAmmounMaxDelta, 0, Form->ScrollAmmountMax);

            r32 ScrollWidth = 15;
            // TODO in future if scrollbars will be separate widgets, they wont work
            FormSize.X = Max(0, FormSize.X - ScrollWidth); 

            rect ScrollBarRect = RectMinSize(V2(FormSize.X, 0), V2(ScrollWidth, FormSize.Y));
            RenderFillRect(ScrollBarRect, 0xffdddddd, &RenderBitmap);

            r32 ScrollScale = MaxHeight / VisibleHeight;  
            r32 ScrollHandleHeight = FormSize.Y * ScrollScale;
            r32 ScrollHandleHeightHalf = ScrollHandleHeight / 2.0f;

            r32 ScrollHandleYMin = (r32)Form->ScrollAmmount * ScrollScale;
            rect ScrollHandleRect = RectMinSize(V2(FormSize.X, ScrollHandleYMin), V2(ScrollWidth, ScrollHandleHeight));
            RenderFillRect(ScrollHandleRect, 0xff999999, &RenderBitmap);

            // TODO remove?
            RenderFillRectOutlineInside(ScrollBarRect, 1.0f, 0xff000000, &RenderBitmap);
          }
          switch(Form->Layout)
          {
            case UIFormLayout_TopToBottom:
              {
                ScrollOffset.Y = (r32)-Form->ScrollAmmount;
              } break;
            case UIFormLayout_BottomToTop:
              {
                ScrollOffset.Y = (r32)Form->ScrollAmmountMax -(r32)Form->ScrollAmmount;
              } break;
          }
        } break;
        // TODO merge with horizontal?
      case UIFormLayout_LeftToRight:
      {
          r32 VisibleWidth = Form->VisibleSize.X;
          r32 MaxWidth = FormSize.X;

          s32 PrevScrollAmmountMax = Form->ScrollAmmountMax;
          Form->ScrollAmmountMax = (s32)Max(0, VisibleWidth - MaxWidth);
          s32 ScrollAmmounMaxDelta = Form->ScrollAmmountMax - PrevScrollAmmountMax;

          if(Form->ScrollAmmountMax == 0)
          {
            Form->ScrollAmmount = 0;
          }
          else
          {
            Form->ScrollAmmount = Clamp(Form->ScrollAmmount + ScrollAmmounMaxDelta, 0, Form->ScrollAmmountMax);

            r32 ScrollHeight = 15;
            r32 ScrollBarMinY = FormSize.Y - ScrollHeight;
            

            // TODO in future if scrollbars will be separate widgets, they wont work
            FormSize.Y = Max(0, FormSize.Y - ScrollHeight); 
              
            rect ScrollBarRect = RectMinSize(V2(0, ScrollBarMinY), V2(FormSize.X, FormSize.Y));
            RenderFillRect(ScrollBarRect, 0xffdddddd, &RenderBitmap);

            r32 ScrollScale = MaxWidth / VisibleWidth;  
            r32 ScrollHandleWidth = FormSize.X * ScrollScale;
            r32 ScrollHandleWidthHalf = ScrollHandleWidth / 2.0f;

            r32 ScrollHandleXMin = (r32)Form->ScrollAmmount * ScrollScale;
            rect ScrollHandleRect = RectMinSize(V2(ScrollHandleXMin, ScrollBarMinY), V2(ScrollHandleWidth, FormSize.Y));
            RenderFillRect(ScrollHandleRect, 0xff999999, &RenderBitmap);

            RenderFillRectOutlineInside(ScrollBarRect, 1.0f, 0xff000000, &RenderBitmap);
          }

          ScrollOffset.X = (r32)-Form->ScrollAmmount;
      } break;
        InvalidDefaultCase;
    }

    // for scrollbar and checking mouse inside
    Form->Position = FormPosition; 
    Form->VisibleSize = FormSize; 
    Form->VisibleRect = RectMinSize(FormPosition, FormSize);
    
    b32 FormActive = RectPointInside(Form->VisibleRect, Window->CursorPosition);
    if(FormActive)
    {
      Form->ScrollAmmount = Clamp(Form->ScrollAmmount + Window->ScrollOffset, 0, Form->ScrollAmmountMax);
    }

    v2 VisibleSize = V2(0.0f);
    v2 Pivot = ScrollOffset;
    switch(Form->Layout)
    {
      case UIFormLayout_TopToBottom:
      case UIFormLayout_LeftToRight:
        {
          Pivot += V2(0.0f);
        } break;
      case UIFormLayout_BottomToTop:
        {
          Pivot += V2(0.0f, FormSize.Y);
        } break;
        InvalidDefaultCase;
    }

    memory_temporary TempWidgetMemory = MemoryPoolBeginTemporaryMemory(&Window->MemoryPool);
    u32 BufferSizeMax = (u32)MemoryPoolSizeRemaining(&Window->MemoryPool);
    u32 BufferSize = 0;
    char *Buffer = (char *)MemoryPoolPushSize(&Window->MemoryPool, BufferSize); 

    for(ui_widget *Widget = Form->Range.First;
        Widget;)
    { 
      v2 WidgetSizeMax = FormSize - 2.0f * V2(Widget->OutlineThickness);
      v2 WidgetSize = {};

      switch(Widget->Type)
      {
        case UIWidgetType_InputBox:
          {
            ui_input_box *InputBox = UIGetInputBox(Widget);
            WidgetSize = ResourceTextSize(Font, InputBox->CharBuffer->String, WidgetSizeMax.X);
          } break;
        case UIWidgetType_ButtonBox:
          {
            ui_button_box *ButtonBox = UIGetButtonBox(Widget);
            WidgetSize = ResourceTextSize(Font, ButtonBox->Text, WidgetSizeMax.X);
          } break;
        case UIWidgetType_TextBox:
          {
            ui_text_box *TextBox = UIGetTextBox(Widget);
            WidgetSize = ResourceTextSize(Font, TextBox->Text, WidgetSizeMax.X);
          } break;
        case UIWidgetType_Room:
          {
            ui_room *Room = UIGetRoom(Widget);
            WidgetSize = ResourceTextSize(Font, Room->Text, WidgetSizeMax.X);
          } break;
        case UIWidgetType_Message:
          {
            ui_message *Message = UIGetMessage(Widget);
            BufferSize = StringFormat(Buffer, BufferSizeMax, "[(s)]: (s)", Message->Username, Message->Message);

            WidgetSize = ResourceTextSize(Font, StringDataLength(Buffer, BufferSize), WidgetSizeMax.X);
          } break;
      }

      r32 WidthMin = 0.0f;
      switch(Widget->WidthMinType)
      {
        case UISizeType_Absolute:
          {
            WidthMin = Widget->WidthMin;
          } break;
        case UISizeType_Relative:
          {
            WidthMin = Widget->WidthMin * WidgetSizeMax.X;
          } break;
        case UISizeType_Fill:
          {
            WidthMin = Max(0, WidgetSizeMax.X - Pivot.X);
          } break;
      }
      WidgetSize.X = Max(WidgetSize.X, WidthMin);

      r32 HeightMin = 0.0f;
      switch(Widget->HeightMinType)
      {
        case UISizeType_Absolute:
          {
            HeightMin = Widget->HeightMin;
          } break;
        case UISizeType_Relative:
          {
            HeightMin = Widget->HeightMin * WidgetSizeMax.Y;
          } break;
        case UISizeType_Fill:
          {
            HeightMin = Max(0, WidgetSizeMax.Y - Pivot.Y);
          } break;
      }
      WidgetSize.Y = Max(WidgetSize.Y, HeightMin);
      v2 WidgetSizeTotal = WidgetSize + V2(2.0f * Widget->OutlineThickness);
      
      // move for next
      switch(Form->Layout)
      {
        case UIFormLayout_TopToBottom:
        case UIFormLayout_BottomToTop:
          {
            VisibleSize.X = Max(VisibleSize.X, WidgetSizeTotal.X);
            VisibleSize.Y += WidgetSizeTotal.Y;
          } break;
        case UIFormLayout_LeftToRight:
          {
            VisibleSize.Y = Max(VisibleSize.Y, WidgetSizeTotal.Y);
            VisibleSize.X += WidgetSizeTotal.X;
          } break;
          InvalidDefaultCase;
      }
      
      rect OutlineRect = {};
      switch(Form->Layout)
      {
        case UIFormLayout_TopToBottom:
          {
            OutlineRect = RectMinSize(Pivot, WidgetSizeTotal);
            Pivot.Y += WidgetSizeTotal.Y;
          } break;
        case UIFormLayout_BottomToTop:
          {
            OutlineRect = RectMinSize(Pivot - V2(0.0f, WidgetSizeTotal.Y), WidgetSizeTotal);
            Pivot.Y -= WidgetSizeTotal.Y;
          } break;
        case UIFormLayout_LeftToRight:
          {
            OutlineRect = RectMinSize(Pivot, WidgetSizeTotal);
            Pivot.X += WidgetSizeTotal.X;
          } break;
          InvalidDefaultCase;
      }
      Widget->VisibleRect = OutlineRect;
  

      // check for hoverness and activness
      if(FormActive)
      {
        v2 LocalCursorPos = Window->CursorPosition - Form->Position;
        if(RectPointInside(Widget->VisibleRect, LocalCursorPos))
        {
          UISetWidgetEvent(Window, Widget, UIWidgetEvent_Hovered);

          if(Window->MouseDown)
          {
            UISetWidgetEvent(Window, Widget, UIWidgetEvent_Pressed);
          }
        }
      }

      RenderFillRectOutlineInside(OutlineRect, Widget->OutlineThickness, Widget->OutlineColor, &RenderBitmap); 
      
      rect InnerRect = RectShrinkSize(OutlineRect, 2.0f * V2(Widget->OutlineThickness)); 
      RenderFillRect(InnerRect, Widget->FillColor, &RenderBitmap); 
      
      if(WidgetSize.X)
      {
        switch(Widget->Type)
        {
          case UIWidgetType_InputBox:
            {
              ui_input_box *InputBox = UIGetInputBox(Widget);

              string InputString = InputBox->CharBuffer->String;
              v2 OutBoxSize;
              u32 OutCharCount = ResourceTextLengthToFitSize(Font, InputString, WidgetSize, &OutBoxSize);
              RenderTextToOutput(Font, InputString, 0xffffffff, InnerRect.Min, WidgetSize.X, &RenderBitmap);

              if(Widget == Window->WidgetWithState[UIWidgetState_Active])
              {
                SYSTEMTIME SystemTime;
                GetSystemTime(&SystemTime);

                // TODO make it persist while writing!
                if(SystemTime.wMilliseconds <= 500)
                {
                  v2 CursorMin = InnerRect.Min + V2(OutBoxSize.X, 0.0f);
                  v2 CursorMax = CursorMin + V2(5.0f, OutBoxSize.Y);
                  RenderFillRect(RectMinMax(CursorMin, CursorMax) , 0xffffffff, &RenderBitmap);
                }
              }
            } break;
          case UIWidgetType_ButtonBox:
            {
              ui_button_box *ButtonBox = UIGetButtonBox(Widget);

              RenderTextToOutput(Font, ButtonBox->Text, ButtonBox->TextColor, InnerRect.Min, WidgetSize.X, &RenderBitmap);

            } break;
          case UIWidgetType_TextBox:
            {
              ui_text_box *TextBox = UIGetTextBox(Widget);
              RenderTextToOutput(Font, TextBox->Text, TextBox->TextColor, InnerRect.Min, WidgetSize.X, &RenderBitmap);

            } break;
          case UIWidgetType_Room:
            {
              ui_room *Room = UIGetRoom(Widget);
              RenderTextToOutput(Font, Room->Text, 0xffffffff, InnerRect.Min, WidgetSize.X, &RenderBitmap);

              if(WidgetSize.X)
              {
              }
            } break;
          case UIWidgetType_Message:
            {
              ui_message *Message = UIGetMessage(Widget);
              RenderTextToOutput(Font, StringDataLength(Buffer, BufferSize), 0xffffffff, InnerRect.Min, WidgetSize.X, &RenderBitmap);

            } break;

            InvalidDefaultCase;
        }
      }
      MemoryPoolResetTemporaryMemory(TempWidgetMemory);

      Widget = Widget->Next;
    }
    Form->VisibleSize = VisibleSize;

    Form = Form->Next;
  }
  

    
  if(Window->MouseDown)
  {
    ui_widget *ActiveWidget = Window->WidgetWithState[UIWidgetState_Active];
    ui_widget *PressedWidget = Window->WidgetWithEvent[UIWidgetEvent_Pressed];
    
    if(!PressedWidget)
    {
      if(ActiveWidget)
      {
        UIResetWidgetState(Window, ActiveWidget, UIWidgetState_Active);
      }
    }
    else
    {
      if(PressedWidget == ActiveWidget)
      {
        // fine
      }
      else
      {
        if(ActiveWidget)
        {
          UIResetWidgetState(Window, ActiveWidget, UIWidgetState_Active);
        }
        UISetWidgetState(Window, PressedWidget, UIWidgetState_Active);
      }
    }
  }

  Window->ScrollOffset = 0;
  Window->MouseDown = false;
}


void
UIPopAll(ui_form *Form)
{
  ui_window *Window = Form->Window;

  for(ui_widget_type PopType = UIWidgetType_First;
      PopType <= UIWidgetType_Last;
      PopType = (ui_widget_type)(PopType + 1))
  {
    ui_range_widget *PopRange = Form->TypeRange + PopType;
    for(ui_widget *PopWidget = PopRange->First;
        PopWidget;)
    {
      switch(PopType)
      {
        case UIWidgetType_InputBox:
          {
            ui_char_buffer *CharBuffer = UIGetInputBox(PopWidget)->CharBuffer;
            if(CharBuffer != &Window->EmptyCharBuffer)
            {
              CharBuffer->NextFree = Window->FirstFreeCharBuffer;
              Window->FirstFreeCharBuffer = CharBuffer;
            }
          } break;
        case UIWidgetType_ButtonBox:
        case UIWidgetType_TextBox:
        case UIWidgetType_Room:
          {
          } break;
          InvalidDefaultCase;
      };
      
      ui_widget *TypeNext = PopWidget->TypeNext;

      PopWidget->TypeNext = Window->FirstFreeWidget[PopType];
      Window->FirstFreeWidget[PopType] = PopWidget;

      PopWidget = TypeNext;
    }
  }

  MemoryZeroStruct(&Form->Range);
  MemoryZeroStruct(&Form->TypeRange);
}
void
UIPopSubsequentForms(ui_form *Form)
{
  ui_window *Window = Form->Window;

  ui_form *FirstForm = Form->Next;
  if(FirstForm)
  {
    Form->Next = 0;

    ui_form *LastForm = FirstForm;
    while(true)
    {
      UIPopAll(LastForm);

      if(!LastForm->Next)
      {
        break;
      }
      else
      {
        LastForm = LastForm->Next;
      }
    }

    LastForm->NextFree = Window->FirstFreeForm;
    Window->FirstFreeForm = FirstForm;

    Window->LastForm = Form;
  }
}

void
UIPopAll(ui_window *Window)
{
  MemoryPoolReset(&Window->MemoryPool);
  
  uptr Offset = offsetof(ui_window, FirstFreeForm);
  MemoryZero((u8 *)Window + Offset, sizeof(ui_window) - Offset);
}

void
UIPopAllOfType(ui_form *Form, ui_widget_type Type)
{
  ui_window *Window = Form->Window;

  ui_range_widget *Range = &Form->Range; 
  ui_range_widget *TypeRange = Form->TypeRange + Type;

  if(Range->First && TypeRange->First)
  {
    if(Range->First->Type == Type)
    {
      Range->First = TypeRange->Last->Next; 
    }
    if(Range->Last->Type == Type)
    {
      Range->Last = TypeRange->First->Prev; 
    }

    for(ui_widget *Pop = TypeRange->First;
        Pop;)
    {
      ui_widget *Prev = Pop->Prev;
      ui_widget *Next = Pop->Next;
      if(Prev)
      {
        Prev->Next = Next;
      }
      if(Next)
      {
        Next->Prev = Prev;
      }

      ui_widget *TypeNext = Pop->TypeNext;

      Pop->TypeNext = Window->FirstFreeWidget[Type];
      Window->FirstFreeWidget[Type] = Pop;

      Pop = TypeNext;
    }
    *TypeRange = {};
  }
}

void
UIPopAllOfType(ui_window *Window, ui_widget_type Type)
{
  for(ui_form *Form = Window->FirstForm;
      Form;)
  {
    UIPopAllOfType(Form, Type);
    Form = Form->Next;
  }
}
