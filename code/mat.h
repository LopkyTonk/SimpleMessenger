#pragma once
struct v2
{
  r32 X;
  r32 Y;
};

v2
V2()
{
  v2 Result;
  Result.X = 0.0f;
  Result.Y = 0.0f;

  return Result;
}


v2
V2(r32 Scalar)
{
  v2 Result;
  Result.X = Scalar;
  Result.Y = Scalar;

  return Result;
}

v2
V2(r32 X, r32 Y)
{
  v2 Result;
  Result.X = X;
  Result.Y = Y;

  return Result;
}

v2 
V2S32(s32 X, s32 Y)
{
  v2 Result;
  Result.X = (r32)X;
  Result.Y = (r32)Y;

  return Result;

}

v2 
V2U32(u32 X, u32 Y)
{
  v2 Result;
  Result.X = (r32)X;
  Result.Y = (r32)Y;

  return Result;

}
 

v2
operator*(r32 Scalar, v2 Vec)
{
  v2 Result;
  Result.X = Scalar * Vec.X;
  Result.Y = Scalar * Vec.Y;

  return Result;
}

v2 
operator-(v2 V)
{
  v2 Result;
  Result.X = -V.X;
  Result.Y = -V.Y;

  return Result;
}

v2
operator-(v2 Vector, r32 Scalar)
{
  v2 Result;
  Result.X = Vector.X - Scalar;
  Result.Y = Vector.Y - Scalar;

  return Result;
}

v2
operator+(v2 Vector, r32 Scalar)
{
  v2 Result;
  Result.X = Vector.X + Scalar;
  Result.Y = Vector.Y + Scalar;

  return Result;
}

v2
operator-(v2 Lhs, v2 Rhs)
{
  v2 Result;
  Result.X = Lhs.X - Rhs.X;
  Result.Y = Lhs.Y - Rhs.Y;

  return Result;
}
v2&
operator-=(v2& Lhs, v2 Rhs)
{
  Lhs.X -= Rhs.X;
  Lhs.Y -= Rhs.Y;

  return Lhs;
}

v2 
operator+(v2 V)
{
  v2 Result;
  Result.X = +V.X;
  Result.Y = +V.Y;

  return Result;
}

v2&
operator+=(v2 &Lhs, v2 Rhs)
{
  Lhs.X += Rhs.X;
  Lhs.Y += Rhs.Y;

  return Lhs;
}

v2
operator+(v2 Lhs, v2 Rhs)
{
  v2 Result;
  Result.X = Lhs.X + Rhs.X;
  Result.Y = Lhs.Y + Rhs.Y;

  return Result;
}


v2
Hadamard(v2 Lhs, v2 Rhs)
{
  v2 Result;
  Result.X = Lhs.X * Rhs.X;
  Result.Y = Lhs.Y * Rhs.Y;

  return Result;
}

struct rect
{
  v2 Min;
  v2 Max;
};


rect
RectMinMax(r32 MinX, r32 MinY, r32 MaxX, r32 MaxY)
{
  rect Result;
  Result.Min = V2(MinX, MinY);
  Result.Max = V2(MaxX, MaxY);

  return Result;
}

rect
RectMinMax(v2 Min, v2 Max)
{
  rect Result;
  Result.Min = Min;
  Result.Max = Max;

  return Result;
}

rect
RectCenterSize(v2 Center, v2 Size)
{
  rect Result;
  Result.Min = Center - (0.5f * Size);
  Result.Max = Center + (0.5f * Size);

  return Result;
}
rect
RectMinSize(v2 Min, v2 Size)
{
  rect Result;
  Result.Min = Min;
  Result.Max = Min + Size;

  return Result;
}

r32 
RectWidth(rect R)
{
  r32 Result = R.Max.X - R.Min.X;
  return Result;
}
r32 
RectHeight(rect R)
{
  r32 Result = R.Max.Y - R.Min.Y;
  return Result;
}
v2 RectSize(rect R)
{
  v2 Result;
  Result.X = RectWidth(R);
  Result.Y = RectHeight(R);

  return Result;
}

b32 RectHasArea(rect Rect)
{
  b32 Result = (Rect.Min.X < Rect.Max.X &&
                Rect.Min.Y < Rect.Max.Y);

  return Result;
}

v2 
RectClampPoint(rect R, v2 P)
{
  v2 Result;
  Result.X = Clamp(P.X, R.Min.X, R.Max.X);
  Result.Y = Clamp(P.Y, R.Min.Y, R.Max.Y);

  return Result;
}

b32
RectPointInside(rect R, v2 P)
{
   b32 Result = (P.X >= R.Min.X && P.X < R.Max.X && P.Y >= R.Min.Y && P.Y < R.Max.Y);
   return Result;
}

rect
RectSetIntersection(rect RectA, rect RectB)
{
  rect Result;
  Result.Min.X = Max(RectA.Min.X, RectB.Min.X);
  Result.Min.Y = Max(RectA.Min.Y, RectB.Min.Y);
  Result.Max.X = Min(RectA.Max.X, RectB.Max.X);
  Result.Max.Y = Min(RectA.Max.Y, RectB.Max.Y);

  return Result;
}

rect
RectSetUnion(rect RectA, rect RectB)
{
  rect Result;
  Result.Min.X = Min(RectA.Min.X, RectB.Min.X);
  Result.Min.Y = Min(RectA.Min.Y, RectB.Min.Y);
  Result.Max.X = Max(RectA.Max.X, RectB.Max.X);
  Result.Max.Y = Max(RectA.Max.Y, RectB.Max.Y);

  return Result;
}

rect
RectShrinkSize(rect RectA, v2 Size)
{
  rect Result;
  Result.Min = RectA.Min + 0.5f * Size;
  Result.Max = RectA.Max - 0.5f * Size;

  return Result;
}
