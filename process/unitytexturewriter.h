/**
 C Utility for writing out 2D or 3D Unity Texture Assets.
   https://gist.github.com/cnlohr/c88980e560ecb403cae6c6525b05ab2f

 2021 <>< Charles Lohr - this code may be distributed under any of he following
 licenses:  CC0, MIT/x11, NewBSD licenses.  Portions may be copied freely to the
 extent allowable by law, no attribution required.

Example usage:
// Execute this with `tcc testgen.c -run`

#include <stdio.h>
#include "unityassettexture.h"
#include <math.h>

int main()
{
	float asset3d[50][50][50][4] = {0};
	int x, y, z;
	for( z = 0; z < 50; z++ )
	for( y = 0; y < 50; y++ )
	for( x = 0; x < 50; x++ )
	{
		float lx = (x - 25.)/25. + sin(z*.1)*.5;
		float ly = (y - 25.)/25. + cos(z*.1)*.5;
		float lz = cos(z*.1);
		float col = 1.-sqrt(lx*lx+ly*ly+lz*lz);
		asset3d[z][y][x][0] = col-10;
		asset3d[z][y][x][1] = col;
		asset3d[z][y][x][2] = col-20;
		asset3d[z][y][x][3] = 255;
	}
	WriteUnityImageAsset( "test3d.asset", asset3d, sizeof(asset3d), 50, 50, 50, UTE_RGBA_FLOAT | UTE_FLAG_IS_3D );
	
	float asset2d[20][20][4] = {0};
	for( y = 0; y < 20; y++ )
	for( x = 0; x < 20; x++ )
	{
		asset2d[y][x][0] = fmod( x+y*9, 1.555 );;
		asset2d[y][x][1] = fmod( x+y*9, 1.455 );;
		asset2d[y][x][2] = fmod( x+y*9, 1.355 );;
		asset2d[y][x][3] = 255;		
	}
	WriteUnityImageAsset( "test2d.asset", asset2d, sizeof(asset2d), 20, 20, 0, UTE_RGBA_FLOAT );
}

*/

#ifndef _UNITYTEXTUREWRITER_H
#define _UNITYTEXTUREWRITER_H

#include <stdint.h>

// See https://github.com/Unity-Technologies/UnityCsReference/blob/master/Runtime/Export/Graphics/GraphicsEnums.cs

#define UTE_UNSPPORTED             0
#define UTE_ALPHA8                 1
#define UTE_ARGB                   2  // Or BGRA4 UNORM (16 bpp)
#define UTE_RGB24                  3
#define UTE_RGBA32                 4
#define UTE_ARGB32                 5
#define UTE_ARGB_FLOAT             6
#define UTE_RGB16                  7 // R5G6B5
#define UTE_BGR24                  8
#define UTE_R16                    9
#define UTE_RGBDXT1                10
#define UTE_RGBADXT3               11
#define UTE_RGBADXT5               12
#define UTE_RGBA16                 13 // Really RGBA4444
#define UTE_BGRA32                 14
#define UTE_R_HALF                 15
#define UTE_RG_HALF                16
#define UTE_RGBA_HALF              17
#define UTE_R_FLOAT                18
#define UTE_RG_FLOAT               19
#define UTE_RGBA_FLOAT             20
#define UTE_YUYV                   21
#define UTE_RGB9e5_SHARED_EXPONENT 22
#define UTE_RGB_FLOAT              23
#define UTE_RGB_HDR_BC6H           24
#define UTE_RGBA_BC7               25
#define UTE_R_BC4                  26
#define UTE_RG_BC5                 27
#define UTE_DXT1_UNORM             28
#define UTE_DXT5_UNORM             29
#define UTE_RGB_PVRTC_2BPP_UNORM   30
#define UTE_RGBA_PVRTC_2BPP_UNORM  31
#define UTE_RGB_PVRTC_4BPP_UNORM   32
#define UTE_RGBA_PVRTC_4BPP_UNORM  33
#define UTE_RGB_ETC_UNORM          34
#define UTE_R_EAC_UNORM            41
#define UTE_R_EAC_SNORM            42
#define UTE_RG_EAC_UNORM           43
#define UTE_RG_EAC_SNORM           44
#define UTE_RGB_ETC2_UNORM         45
#define UTE_RGBA_ETC2_PUNCHTHROUGH_UNORM        46
#define UTE_RGBA_ETC2_UNORM        47
#define UTE_RGBA_ATSC4X4_UNORM     48
#define UTE_RGBA_ATSC5x5_UNORM     49
#define UTE_RGBA_ATSC6x6_UNORM     50
#define UTE_RGBA_ATSC8x8_UNORM     51
#define UTE_RGBA_ATSC10x10_UNORM   52
#define UTE_RGBA_ATSC12x12_UNORM   53
#define UTE_RGBA_ATSC4x4B_UNORM    54
#define UTE_RGBA_ATSC5x5B_UNORM    55
#define UTE_RGBA_ATSC6x6B_UNORM    56
#define UTE_RGBA_ATSC8x8B_UNORM    57
#define UTE_RGBA_ATSC10x10B_UNORM  58
#define UTE_RGBA_ATSC12x12B_UNORM  59
#define UTE_RGB_ETCB_UNORM         60
#define UTE_RGBA_ETC2B_UNORM       61
#define UTE_RG8_UNORM              62
#define UTE_R8_UNORM               63
#define UTE_RGB_ETC_B_UNORM        64


// Note: For these formats, your object size must be correct.  These are raw 16-bit formats.
#define UTE_RG32_UNORM             72
#define UTE_RGB48_UNORM            73
#define UTE_RGBA64_UNORM           74




#define UTE_FLAG_FORMAT_MASK             0xff

#define UTE_FLAG_ALPHA_IS_TRANSPARENCY   0x100
#define UTE_FLAG_FILTER_MODE_LINEAR      0x200

#define UTE_FLAG_FILTER_MODE_CLAMP_U     0x1000
#define UTE_FLAG_FILTER_MODE_CLAMP_V     0x2000
#define UTE_FLAG_FILTER_MODE_CLAMP_W     0x4000

#define UTE_FLAG_IS_3D                   0x10000

static inline void WriteUnityImageAsset( const char * filename, void * data, int bytes, int width, int height, int depth, uint32_t flags )
{
	FILE * f = fopen( filename, "w" );
	fprintf( f, "%%YAML 1.1\n\
%%TAG !u! tag:unity3d.com,2011:\n\
--- %s\n\
Texture%dD:\n\
  m_ObjectHideFlags: 0\n\
  m_CorrespondingSourceObject: {fileID: 0}\n\
  serializedVersion: 2\n\
  m_Width: %d\n\
  m_Height: %d\n\
  m_Depth: %d\n\
  m_CompleteImageSize: %d\n\
  %s: %d\n\
  m_MipCount: 1\n\
  m_IsReadable: 1\n\
  m_AlphaIsTransparency: %d\n\
  m_ImageCount: 1\n\
  m_TextureDimension: %d\n\
  m_TextureSettings:\n\
    serializedVersion: 2\n\
    m_FilterMode: %d\n\
    m_Aniso: 0\n\
    m_MipBias: 0\n\
    m_WrapU: %d\n\
    m_WrapV: %d\n\
    m_WrapW: %d\n\
  m_LightmapFormat: 0\n\
  m_ColorSpace: 0\n\
  image data: %d\n\
  _typelessdata: ",
	(flags&UTE_FLAG_IS_3D)?"!u!117 &11700000":"!u!28 &2800000",
	(flags&UTE_FLAG_IS_3D)?3:2, width, height, (flags&UTE_FLAG_IS_3D)?depth:1, bytes,
	(flags&UTE_FLAG_IS_3D)?"m_Format":"m_TextureFormat",
	flags & UTE_FLAG_FORMAT_MASK, !!(flags&UTE_FLAG_ALPHA_IS_TRANSPARENCY),
	(flags&UTE_FLAG_IS_3D)?3:2, (flags&UTE_FLAG_FILTER_MODE_LINEAR)?1:0,
	!!(flags & UTE_FLAG_FILTER_MODE_CLAMP_U),
	!!(flags & UTE_FLAG_FILTER_MODE_CLAMP_V),
	!!(flags & UTE_FLAG_FILTER_MODE_CLAMP_W),
	bytes );

	int i;
	for( i = 0; i < bytes; i++ )
	{
		int value = ((uint8_t*)data)[i];
		fprintf( f, "%02x", value );
	}

	fprintf( f, "\n  m_StreamData:\n\
    offset: 0\n\
    size: 0\n\
    path: \n" );
	fclose( f );
}

#endif
