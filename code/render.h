#define SpecialCharacters " \n"

void
RenderClear(u32 Color, bitmap *Bitmap)
{
  if(Bitmap->Width == Bitmap->Pitch)
  {
    MemorySet(Bitmap->Pixels, 4 * Bitmap->Width * Bitmap->Height, Color);
  }
  else
  {
    for(u32 RowIndex = 0;
        RowIndex < Bitmap->Height;
        ++RowIndex)
    {
      MemorySet(Bitmap->Pixels + RowIndex * Bitmap->Pitch, 4 * Bitmap->Width, Color);
    }
  }
}

void
RenderBlit(bitmap *SrcBmp, u32 SrcColor,
           v2 Offset,
           bitmap *DstBmp)
{
  rect SrcRect = RectMinMax(Offset, V2U32(SrcBmp->Width, SrcBmp->Height) + Offset);
  rect DstRect = RectMinMax(V2(), V2U32(DstBmp->Width, DstBmp->Height));
  
  rect RenderRect = RectSetIntersection(SrcRect, DstRect);

  if(RectHasArea(RenderRect))
  {
    // surely this can be written better
    u32 SrcOffsetX = Offset.X < 0.0f ? (u32)Round(-Offset.X) : 0;
    u32 SrcOffsetY = Offset.Y < 0.0f ? (u32)Round(-Offset.Y) : 0;

    u32 DstOffsetX = Offset.X > 0.0f ? (u32)Round(Offset.X) : 0;
    u32 DstOffsetY = Offset.Y > 0.0f ? (u32)Round(Offset.Y) : 0;

    u32 ClippedSrcWidth = SrcBmp->Width - SrcOffsetX;
    u32 ClippedSrcHeight = SrcBmp->Height - SrcOffsetY;

    u32 ClippedDstWidth = DstBmp->Width - DstOffsetX;
    u32 ClippedDstHeight = DstBmp->Height - DstOffsetY;

    u32 ColCount = Min(ClippedSrcWidth, ClippedDstWidth);
    u32 RowCount = Min(ClippedSrcHeight, ClippedDstHeight);

    u32 *SrcRow = SrcBmp->Pixels + SrcOffsetY * SrcBmp->Pitch + SrcOffsetX;
    u32 *DstRow = DstBmp->Pixels + DstOffsetY * DstBmp->Pitch + DstOffsetX;

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
        u32 SR = *Src >> 0 & 0xff;
        u32 SG = *Src >> 8 & 0xff;
        u32 SB = *Src >> 16 & 0xff;
        u32 SA = *Src >> 24 & 0xff;

        r32 LSR = Square(SR / 255.0f);
        r32 LSG = Square(SG / 255.0f);
        r32 LSB = Square(SB / 255.0f);
        r32 LSA = SA / 255.0f;

        u32 SCR = SrcColor >> 0 & 0xff;
        u32 SCG = SrcColor >> 8 & 0xff;
        u32 SCB = SrcColor >> 16 & 0xff;
        u32 SCA = SrcColor >> 24 & 0xff;

        r32 LSCR = Square(SCR / 255.0f);
        r32 LSCG = Square(SCG / 255.0f);
        r32 LSCB = Square(SCB / 255.0f);
        r32 LSCA = SCA / 255.0f;
        
        LSR *= LSCR;
        LSG *= LSCG;
        LSB *= LSCB;
        LSA *= LSCA;

        LSR *= LSA;
        LSG *= LSA;
        LSB *= LSA;

        u32 DR = *Dst >> 0 & 0xff;
        u32 DG = *Dst >> 8 & 0xff;
        u32 DB = *Dst >> 16 & 0xff;
        u32 DA = *Dst >> 24 & 0xff;

        r32 LDR = Square(DR / 255.0f);
        r32 LDG = Square(DG / 255.0f);
        r32 LDB = Square(DB / 255.0f);
        r32 LDA = DA / 255.0f;

        r32 InvLSA = 1.0f - LSA;
        r32 LR = InvLSA * LDR + LSR;
        r32 LG = InvLSA * LDG + LSG;
        r32 LB = InvLSA * LDB + LSB;
        r32 LA = LSA + LDA - LSA * LDA;

        u32 R = (u32)(Sqrt(LR) * 255.0f + 0.5f) & 0xff;
        u32 G = (u32)(Sqrt(LG) * 255.0f + 0.5f) & 0xff;
        u32 B = (u32)(Sqrt(LB) * 255.0f + 0.5f) & 0xff;
        u32 A = (u32)(LA * 255.0f) & 0xff;

        *Dst = (R << 0) | (G << 8) | (B << 16) | (A << 24);

        ++Src;
        ++Dst;
      }

      SrcRow += SrcBmp->Pitch;
      DstRow += DstBmp->Pitch;
    }
  }
}

void
RenderFillRect(rect FillRect, u32 Color, bitmap *DstBmp)
{
  rect RenderRect = RectSetIntersection(FillRect, RectMinMax(V2(), V2U32(DstBmp->Width, DstBmp->Height)));

  if(RectHasArea(RenderRect))
  {
    u32 MinX = (u32)Round(RenderRect.Min.X);
    u32 MinY = (u32)Round(RenderRect.Min.Y);
    u32 MaxX = (u32)Round(RenderRect.Max.X);
    u32 MaxY = (u32)Round(RenderRect.Max.Y);

    u32 *DstRow = DstBmp->Pixels + MinY * DstBmp->Pitch + MinX;

    u32 RowCount = MaxY - MinY;
    u32 ColCount = MaxX - MinX;
    
    for(u32 RowIndex = 0;
        RowIndex < RowCount;
        ++RowIndex)
    {
      MemorySet(DstRow, sizeof(u32) * ColCount, Color);

      DstRow += DstBmp->Pitch;
    }
  }
}

void
RenderFillRectOutlineOutside(rect OutlineRect, r32 Thickness, u32 Color, bitmap *DstBitmap)
{
  v2 TopLeft = OutlineRect.Min - V2(Thickness);
  v2 BottomRight = OutlineRect.Max + V2(Thickness);

  v2 TopLineMin = TopLeft;
  v2 TopLineMax = V2(BottomRight.X, TopLeft.Y + Thickness); 
  RenderFillRect(RectMinMax(TopLineMin, TopLineMax), Color, DstBitmap);
  
  v2 BottomLineMin = V2(TopLeft.X, BottomRight.Y - Thickness);
  v2 BottomLineMax = BottomRight; 
  RenderFillRect(RectMinMax(BottomLineMin, BottomLineMax), Color, DstBitmap);

  v2 LeftLineMin = V2(TopLeft.X, TopLeft.Y + Thickness);
  v2 LeftLineMax = V2(TopLeft.X + Thickness, BottomRight.Y - Thickness);
  RenderFillRect(RectMinMax(LeftLineMin, LeftLineMax), Color, DstBitmap);
  
  v2 RightLineMin = V2(BottomRight.X - Thickness, TopLeft.Y + Thickness);
  v2 RightLineMax = V2(BottomRight.X, BottomRight.Y - Thickness);
  RenderFillRect(RectMinMax(RightLineMin, RightLineMax), Color, DstBitmap);
}
void
RenderFillRectOutlineInside(rect OutlineRect, r32 Thickness, u32 Color, bitmap *DstBitmap)
{
  v2 TopLeft = OutlineRect.Min;
  v2 BottomRight = OutlineRect.Max;

  v2 TopLineMin = TopLeft;
  v2 TopLineMax = V2(BottomRight.X, TopLeft.Y + Thickness); 
  RenderFillRect(RectMinMax(TopLineMin, TopLineMax), Color, DstBitmap);
  
  v2 BottomLineMin = V2(TopLeft.X, BottomRight.Y - Thickness);
  v2 BottomLineMax = BottomRight; 
  RenderFillRect(RectMinMax(BottomLineMin, BottomLineMax), Color, DstBitmap);

  v2 LeftLineMin = V2(TopLeft.X, TopLeft.Y + Thickness);
  v2 LeftLineMax = V2(TopLeft.X + Thickness, BottomRight.Y - Thickness);
  RenderFillRect(RectMinMax(LeftLineMin, LeftLineMax), Color, DstBitmap);
  
  v2 RightLineMin = V2(BottomRight.X - Thickness, TopLeft.Y + Thickness);
  v2 RightLineMax = V2(BottomRight.X, BottomRight.Y - Thickness);
  RenderFillRect(RectMinMax(RightLineMin, RightLineMax), Color, DstBitmap);
}

// no special chars are treated here
void  
RenderTextToOutput_(resource_font *Font, string Text, u32 Color, v2 StartOffset, bitmap *DstBmp)
{
  v2 Offset = StartOffset;

  for(u32 LetterIndex = 0;
      LetterIndex < Text.Length;
      ++LetterIndex)
  {
    char Codepoint = Text.Data[LetterIndex];
    if(ResourceCodepointPresent(Font, Codepoint))
    {
      font_glyph *Glyph = ResourceGlyphFromCodepoint(Font, Codepoint);
      resource_bitmap *ResBmp = &Glyph->ResourceBitmap;

      Assert(ResBmp);
      bitmap *SrcBmp = &ResBmp->Bitmap;
      // TODO add align to account 
      // wrapping
      if(Offset.X + SrcBmp->Width > DstBmp->Width) 
      {
        Offset.X = StartOffset.X;
        Offset.Y += Font->LineHeight;

        if(Offset.Y + Font->LineHeight > SrcBmp->Height)
        {
          break;
        }
      }

      v2 BlitOffset = Offset + Hadamard(ResBmp->Align, V2U32(SrcBmp->Width, SrcBmp->Height));
      RenderBlit(SrcBmp, Color, BlitOffset, DstBmp);

      u32 NextLetterIndex = LetterIndex + 1;
      if(NextLetterIndex < Text.Length)
      {
        u32 NextCodepoint = Text.Data[NextLetterIndex];
        if(ResourceCodepointPresent(Font, NextCodepoint))
        {
          r32 KernAdvance = Font->Kernings[Codepoint * Font->MaxCodepoint + NextCodepoint];

          Offset.X += Glyph->VerticalAdvance + KernAdvance;
        }
      }
    }
  }
}

void
RenderTextToOutput(resource_font *Font, string Text, u32 Color, v2 StartOffset, r32 WrapWidth, bitmap *DstBitmap)
{
  Assert(WrapWidth > 0.0f);
  v2 Offset = V2(0.0f);

  auto NextLine = [&]()
  {
    Offset.Y += Font->LineHeight;
    Offset.X = 0.0f;
  };
  while(Text.Length)
  {
    u32 WordLength = StringNextMatchOffset(Text, StringCharArray(" \n"));
    string Word = StringCut(&Text, WordLength);

    r32 WordWidth = ResourceTextWidth(Font, Word);
    if(WordWidth > WrapWidth)
    {
      while(true)
      {
        // newline
        r32 WordBreakWidth;
        u32 WordBreakLength = ResourceTextLengthToFitWidth(Font, Word, WrapWidth - Offset.X, &WordBreakWidth);
        string WordBreak = StringCut(&Word, WordBreakLength);
        RenderTextToOutput_(Font, WordBreak, Color, Offset + StartOffset, DstBitmap);

        if(WordBreak.Length == 0)
        {
          break;
        }
        else if(Word.Length == 0)
        {
          Offset.X = WordBreakWidth;
          break;
        }
        else
        {
          NextLine();
        }
      }
    }
    else  
    {
      if(Offset.X + WordWidth > WrapWidth)
      {
        NextLine();
      }

      RenderTextToOutput_(Font, Word, Color, Offset + StartOffset, DstBitmap);

      Offset.X += WordWidth;
    }

    // try to find beginning of next word
    while(Text.Length)
    {
      char CurrentChar = Text.Data[0];
      if(CurrentChar == ' ')
      {
        Offset.X += ResourceCodepointVerticalAdvance(Font, ' ');
        if(Offset.X > WrapWidth)
        {
          NextLine();
        }
        StringCut(&Text, 1);
      }
      else if (CurrentChar == '\n')
      {
        NextLine();
        StringCut(&Text, 1);
      }
      // this is valid char
      else
      {
        break;
      }
    }
  }

}
