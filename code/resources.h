struct bitmap
{
  // R:0-7 G:8-15 B:16-23 A:24-31
  u32 *Pixels;
  u32 Width;
  u32 Height;
  u32 Pitch;
};

bitmap
Bitmap(u32 *Pixels, u32 Width, u32 Height, u32 Pitch)
{
  bitmap Result;
  Result.Pixels = Pixels;
  Result.Width = Width;
  Result.Height = Height;
  Result.Pitch = Pitch;

  return Result;
}

bitmap
Bitmap(u32 *Pixels, u32 Width, u32 Height)
{
  bitmap Result = Bitmap(Pixels, Width, Height, Width);

  return Result;
}

bitmap 
BitmapView(bitmap *SubBitmap, u32 Width, u32 Height, u32 OffsetX, u32 OffsetY)
{
  Assert(Width <= SubBitmap->Width);
  Assert(Height <= SubBitmap->Height);
  Assert(Width + OffsetX <= SubBitmap->Width);
  Assert(Height + OffsetY <= SubBitmap->Height);
  

  bitmap Result;
  Result.Pixels = (u32 *)SubBitmap->Pixels + OffsetY * SubBitmap->Pitch + OffsetX;
  Result.Width = Width;
  Result.Height = Height;
  Result.Pitch = SubBitmap->Pitch;

  return Result;
}

bitmap 
BitmapView(bitmap *SubBitmap, v2 Size, v2 Offset)
{
  u32 Width = (u32)Floor(Size.X);
  u32 Height = (u32)Floor(Size.Y);
  u32 OffsetX = (u32)Floor(Offset.X);
  u32 OffsetY = (u32)Floor(Offset.Y);

  bitmap Result = BitmapView(SubBitmap, Width, Height, OffsetX, OffsetY);
  return Result;
}

struct resource_bitmap
{ 
  bitmap Bitmap;
  v2 Align;
};

struct font_glyph
{
  u32 Codepoint;
  u32 VerticalAdvance;
  resource_bitmap ResourceBitmap;
};


struct resource_font
{
  r32 LineHeight;
  
  u32 GlyphCount;
  font_glyph *Glyphs;
  
  // TODO Add min codepoint so there wont be empty spaces
  u32 MaxCodepoint;
  // Codepoint to glyph
  u32 *CodepointGlyphMap;
  // 2D array MaxCodepoint * MaxCodepoint
  r32 *Kernings;
};

#define CodepointInvalid (0)

b32 
ResourceCodepointPresent(resource_font *Font, u32 Codepoint)
{
  b32 Result = Codepoint <= Font->MaxCodepoint;
  return Result;
}

u32 
ResourceGlyphIndexFromCodepoint(resource_font *Font, u32 Codepoint)
{
  u32 Result = Font->CodepointGlyphMap[Codepoint];

  return Result;
}

font_glyph * 
ResourceGlyphFromCodepoint(resource_font *Font, u32 Codepoint)
{
  font_glyph *Result = Font->Glyphs + Font->CodepointGlyphMap[Codepoint];

  return Result;
}

r32 
ResourceCodepointKerning(resource_font *Font, u32 Codepoint, u32 NextCodepoint)
{ 
  r32 Result = Font->Kernings[Codepoint * Font->MaxCodepoint + NextCodepoint];
  return Result;
}

r32
ResourceGlyphVerticalAdvance(resource_font *Font, u32 GlyphIndex)
{
  r32 Result = (r32)Font->Glyphs[GlyphIndex].VerticalAdvance;
  return Result;
}

r32
ResourceCodepointVerticalAdvance(resource_font *Font, u32 Codepoint)
{
  r32 Result = ResourceGlyphVerticalAdvance(Font, ResourceGlyphIndexFromCodepoint(Font, Codepoint));
  return Result;
}

r32
ResourceCodepointVerticalAdvance(resource_font *Font, u32 Codepoint, u32 NextCodepoint)
{
  r32 Result = ResourceCodepointVerticalAdvance(Font, Codepoint) + ResourceCodepointKerning(Font, Codepoint, NextCodepoint);

  return Result;
}
r32
ResourceTextWidth(resource_font *Font, string Text)
{
  r32 Result = 0.0f;
  for(u32 LetterIndex = 0;
      LetterIndex < Text.Length;
      ++LetterIndex)
  {
    Result += ResourceCodepointVerticalAdvance(Font, Text.Data[LetterIndex]);
    if(LetterIndex + 1 < Text.Length)
    {
      Result += ResourceCodepointKerning(Font, Text.Data[LetterIndex], Text.Data[LetterIndex + 1]);
    }
  }

  return Result;
}

u32
ResourceTextLengthToFitWidth(resource_font *Font, string Text, r32 MaxWidth, r32 *FitWidth)
{
  u32 Result = 0;
  *FitWidth = 0.0f;

  for(u32 LetterIndex = 0;
        LetterIndex < Text.Length;
        ++LetterIndex)
  {   
    u32 Codepoint = Text.Data[LetterIndex];
    r32 AddWidth = ResourceCodepointVerticalAdvance(Font, Codepoint);
    
    if(LetterIndex + 1 < Text.Length)
    {
      u32 NextCodepoint = Text.Data[LetterIndex + 1];
      AddWidth += ResourceCodepointKerning(Font, Codepoint, NextCodepoint); 
    }

    if(*FitWidth + AddWidth > MaxWidth)
    {
      break;
    }
    else
    {
      *FitWidth += AddWidth;
      ++Result;
    }
  }

  return Result;
}

v2
ResourceTextSize(resource_font *Font, string Text, r32 MaxWidth)
{
  r32 OffsetX = 0.0f;
  v2 Result = V2(0.0f, Font->LineHeight);
  
  auto NewLine = [&]()
  { 
    Result.X = Max(Result.X, OffsetX);
    Result.Y += Font->LineHeight;

    OffsetX = 0.0f;
  };

  while(Text.Length)
  {
    u32 WordLength = StringNextMatchOffset(Text, StringCharArray(" \n"));
    string Word = StringCut(&Text, WordLength);

    r32 WordWidth = ResourceTextWidth(Font, Word);

    if(WordWidth > MaxWidth)
    {
      while(true)
      {
        r32 WordBreakWidth = 0.0f;
        u32 WordBreakLength = ResourceTextLengthToFitWidth(Font, Word, MaxWidth - OffsetX, &WordBreakWidth);
        StringCut(&Word, WordBreakLength);

        // nothing cut
        if(WordBreakLength == 0)
        {
          break;
        }
        else if(Word.Length == 0)
        {
          OffsetX = WordBreakWidth;
          Result.X = Max(Result.X, OffsetX);

          break;
        }
        else
        {
          OffsetX += WordBreakWidth;
          NewLine();
        }
      }
    }
    else  
    {
      if(OffsetX + WordWidth > MaxWidth)
      {
        NewLine();
      }

      OffsetX += WordWidth;
      Result.X = Max(Result.X, OffsetX);
    }

    // try to find beginning of next word
    while(Text.Length)
    {
      char CurrentChar = Text.Data[0];
      if(CurrentChar == ' ')
      {
        OffsetX += ResourceCodepointVerticalAdvance(Font, ' ');
        if(OffsetX > MaxWidth)
        {
          NewLine();
        }
        
        StringCut(&Text, 1);
      }
      else if (CurrentChar == '\n')
      {
        NewLine();

        StringCut(&Text, 1);
      }
      // this is valid char
      else
      {
        break;
      }
    }
  }

  return Result;
}

u32
ResourceTextLengthToFitSize(resource_font *Font, string Text, v2 FitSize, v2 *OutSize)
{
  v2 Size = V2(0.0f, Font->LineHeight);
  u32 Cursor = 0;

  while(true)
  {
    if(Cursor == Text.Length)
    {
      break;
    }
    else if (Text.Data[Cursor] == '\n')
    {
      Size.X = 0.0f;
      Size.Y += Font->LineHeight;
    }
    else
    {
      Size.X += ResourceCodepointVerticalAdvance(Font, Text.Data[Cursor]);
      u32 CursorNext = Cursor + 1;
      if(CursorNext != Text.Length)
      {
        Size.X += ResourceCodepointKerning(Font, Text.Data[Cursor], Text.Data[CursorNext]);
      }
    }

    if(Size.X > FitSize.X || Size.Y > FitSize.Y)
    {
      break;
    }
    else
    {
      ++Cursor;
    }
  }

  if(OutSize)
  {
    *OutSize = Size;
  }
  
  u32 Result = Cursor;
  return Result;
}
