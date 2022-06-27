#pragma once

#define FontHeaderMagicValue (u64(123456789))
struct file_font_header
{
  u32 MagicValue;
  r32 LineHeight;
  u32 GlyphCount;
  u32 MaxCodepoint;

  u64 GlyphOffset; //file_glyph[MaxCodepoint]
  u64 CodepointGlyphMapOffset; //u32[MaxCodepoint]
  u64 KerningsOffset; // r32[MaxCodepoint * MaxCodepoint]
  u64 BitmapOffset;

  u64 GlyphSize;
  u64 CodepointGlyphMapSize;
  u64 KerningsSize;
  u64 BitmapSize;
};

struct file_glyph
{
  u32 Codepoint;
  u32 VerticalAdvance;
  
  u64 BitmapOffset;
  u32 BitmapWidth;
  u32 BitmapHeight;
  r32 BitmapAlignX;
  r32 BitmapAlignY;
};


/*
struct font_glyph
{
  u32 Codepoint;
  u32 VerticalAdvance;
  resource_bitmap ResourceBitmap;
};
*/
