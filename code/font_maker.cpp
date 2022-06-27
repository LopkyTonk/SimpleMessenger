#include "platform.h"
#include "win32.h"
#include "intrinsics.h"
#include "mem.h"
#include "mat.h"
#include "str.h"

#include "resources.h"
#include "file.h"


#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#include "stdio.h"

int main()
{
  u32 CodepointMin = ' ';
  u32 CodepointMax = '~';
  u32 GlyphCount = CodepointMax - CodepointMin + 2;

  uptr FileGlyphsSize = sizeof(file_glyph) * GlyphCount;
  file_glyph *FileGlyphs = (file_glyph *)Win32Alloc(FileGlyphsSize);
  MemoryZero(FileGlyphs, FileGlyphsSize);


#define FontPath "c:/windows/fonts/calibri.ttf"
  FILE *FontFile;
  fopen_s(&FontFile, FontPath, "rb");
  if(!FontFile)
  {
    printf("Unapble to open file %s!", FontPath);
    return -1;
  }

  fseek(FontFile, 0, SEEK_END);
  long FontFileSize = ftell(FontFile);
  fseek(FontFile, 0, SEEK_SET);
  void *FileBuffer = Win32Alloc(FontFileSize);
  fread(FileBuffer, FontFileSize, 1, FontFile);

  stbtt_fontinfo FontInfo_;
  stbtt_fontinfo *FontInfo = &FontInfo_;
  stbtt_InitFont(FontInfo, (unsigned char *)FileBuffer, 0);

  r32 FontHeight = 28;
  r32 FontScale = stbtt_ScaleForPixelHeight(FontInfo, FontHeight); 

  s32 Ascent, Descent, LineGap;
  stbtt_GetFontVMetrics(FontInfo, &Ascent, &Descent, &LineGap);
  r32 LineHeight = FontScale * (Ascent + (-Descent) + LineGap);

  u64 CodepointGlyphMapSize = sizeof(u32) * CodepointMax;
  u32 *CodepointGlyphMap = (u32 *)Win32Alloc(CodepointGlyphMapSize);
  MemorySet(CodepointGlyphMap, CodepointGlyphMapSize, CodepointInvalid);

  u64 KerningsSize = sizeof(r32) * CodepointMax * CodepointMax;
  r32 *Kernings = (r32 *)Win32Alloc(KerningsSize);
  MemoryZero(Kernings, KerningsSize);

  file_font_header FontHeader = {};
  FontHeader.MagicValue = FontHeaderMagicValue;
  FontHeader.LineHeight = LineHeight;
  FontHeader.GlyphCount = GlyphCount;
  FontHeader.MaxCodepoint = CodepointMax; 
  FontHeader.GlyphOffset = sizeof(file_font_header);
  FontHeader.GlyphSize = FileGlyphsSize;
  FontHeader.CodepointGlyphMapOffset = FontHeader.GlyphOffset + FileGlyphsSize;
  FontHeader.CodepointGlyphMapSize = CodepointGlyphMapSize;
  FontHeader.KerningsOffset = FontHeader.CodepointGlyphMapOffset + CodepointGlyphMapSize;
  FontHeader.KerningsSize = KerningsSize;
  FontHeader.BitmapOffset = FontHeader.KerningsOffset + KerningsSize;
  
  u64 BitmapSize = 0;

  {
    FileGlyphs[0] = {};
    u32 Codepoint = ' ';
    for(u32 GlyphIndex = 1;
        GlyphIndex < GlyphCount;
        ++GlyphIndex)
    {
      file_glyph *FileGlyph = FileGlyphs + GlyphIndex;

      s32 X0, Y0, X1, Y1;
      stbtt_GetCodepointBitmapBox(FontInfo, Codepoint, FontScale, FontScale, &X0, &Y0, &X1, &Y1);
      s32 LeftSideBearing, VerticalAdvance;
      stbtt_GetCodepointHMetrics(FontInfo, Codepoint, &VerticalAdvance, &LeftSideBearing);

      FileGlyph->Codepoint = Codepoint;
      FileGlyph->VerticalAdvance = (u32)(FontScale * (r32)VerticalAdvance + 0.5f);
      FileGlyph->BitmapWidth = X1 - X0; 
      FileGlyph->BitmapHeight = Y1 - Y0;
      FileGlyph->BitmapOffset = BitmapSize;
      FileGlyph->BitmapAlignX = SafeDivide0(FontScale * (r32)LeftSideBearing, (r32)FileGlyph->BitmapWidth);

      r32 VerticalOffset = FontScale * (r32)Ascent + (r32)Y0;
      FileGlyph->BitmapAlignY = SafeDivide0(VerticalOffset, (r32)FileGlyph->BitmapHeight);

      BitmapSize += 4 * FileGlyph->BitmapWidth * FileGlyph->BitmapHeight;

      CodepointGlyphMap[Codepoint++] = GlyphIndex;
    }
  }
  FontHeader.BitmapSize = BitmapSize;

  void *BitmapMemory = Win32Alloc(BitmapSize);
  u32 TempBitmapWidth = 256;
  u32 TempBitmapHeight = 256;
  unsigned char *TempBitmap = (unsigned char *)Win32Alloc(TempBitmapWidth * TempBitmapHeight);

  for(u32 GlyphIndex = 1;
      GlyphIndex < GlyphCount;
      ++GlyphIndex)
  {
    file_glyph *FileGlyph = FileGlyphs + GlyphIndex;

    MemoryZero(TempBitmap, TempBitmapWidth * TempBitmapHeight);
    stbtt_MakeCodepointBitmap(FontInfo, TempBitmap, FileGlyph->BitmapWidth, FileGlyph->BitmapHeight, TempBitmapWidth, FontScale, FontScale, FileGlyph->Codepoint);

    u8 *SrcRow = (u8 *)TempBitmap;
    u32 *DstRow = (u32 *)((u8 *)BitmapMemory + FileGlyph->BitmapOffset);
    for(u32 Y = 0;
        Y < FileGlyph->BitmapHeight;
        ++Y)
    {
      u8 *Src = SrcRow;
      u32 *Dst = DstRow;

      for(u32 X = 0;
          X < FileGlyph->BitmapWidth;
          ++X)
      {
        u32 Alpha = (u32)(*Src++) & 0xff;
        *Dst++ = 0x00ffffff | (Alpha << 24);
      }


      SrcRow += TempBitmapWidth;
      DstRow += FileGlyph->BitmapWidth;
    }

    FileGlyph->BitmapOffset += FontHeader.KerningsOffset;
  }


  for(u32 GlyphIndex = 0;
      GlyphIndex < GlyphCount;
      ++GlyphIndex)
  {
    u32 Codepoint = FileGlyphs[GlyphIndex].Codepoint;

    for(u32 NextGlyphIndex = 0;
        NextGlyphIndex < GlyphCount;
        ++NextGlyphIndex)
    {
      u32 NextCodepoint = FileGlyphs[NextGlyphIndex].Codepoint;
      Kernings[Codepoint * CodepointMax + NextCodepoint] = FontScale * (r32)stbtt_GetCodepointKernAdvance(FontInfo, Codepoint, NextCodepoint);
    }
  }

  
#define OutPath "font_file.ff"
  FILE *OutFile;
  fopen_s(&OutFile, OutPath, "wb");
  if(!OutFile)
  {
    printf("Unapble to open file %s!", OutPath);
    return -1;
  }
  
  auto FileWriteData = [&OutFile](u64 Offset, void *SrcData, u32 SrcSize)
  {
    fseek(OutFile, (long)Offset, SEEK_SET);
    fwrite(SrcData, SrcSize, 1, OutFile);
  };
  FileWriteData(0, &FontHeader, (u32)sizeof(FontHeader));
  FileWriteData(FontHeader.GlyphOffset, FileGlyphs, (u32)FileGlyphsSize);
  FileWriteData(FontHeader.CodepointGlyphMapOffset, CodepointGlyphMap, (u32)CodepointGlyphMapSize);
  FileWriteData(FontHeader.KerningsOffset, Kernings, (u32)KerningsSize);
  FileWriteData(FontHeader.BitmapOffset, BitmapMemory, (u32)BitmapSize);
}
