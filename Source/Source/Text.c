#include <Text.h>
#include <FileSystem.h>
#include <Log.h>
#include <string.h>

KMSTRIPHEAD	TEX_StripHead;
KMVERTEX_16 TEX_TextBuffer[ 64 ];

#define BMF_ID_INFO		1
#define BMF_ID_COMMON	2
#define BMF_ID_PAGES	3
#define BMF_ID_CHARS	4
#define BMF_ID_KERNING	5

#define BMF_COMMON_PACKED_SIZE	15

/* Unsuccessful in my attempt to find out how to #pragma pack this, I've
 * resorted to using chars to pack it to a one-byte boundary */
typedef struct _tagBMF_CHUNK
{
	unsigned char	ID;
	unsigned char	Size[ 4 ];
}BMF_CHUNK;

typedef struct _tagBMF_COMMON
{
	Uint16	LineHeight;
	Uint16	BaseLine;
	Uint16	Width;
	Uint16	Height;
	Uint16	Pages;
	Uint8	BitField;
	Uint8	Alpha;
	Uint8	Red;
	Uint8	Green;
	Uint8	Blue;
}BMF_COMMON;

typedef struct _tagBMF_CHAR
{
	Uint32	ID;
	Uint16	X;
	Uint16	Y;
	Uint16	Width;
	Uint16	Height;
	Sint16	XOffset;
	Sint16	YOffset;
	Sint16	XAdvance;
	Uint8	Page;
	Uint8	Channel;
}BMF_CHAR;

int TEX_Initialise( void )
{
	KMPACKEDARGB BaseColour;
	KMSTRIPCONTEXT TextContext;

	memset( &TextContext, 0, sizeof( KMSTRIPCONTEXT ) );

	TextContext.nSize = sizeof( TextContext );
	TextContext.StripControl.nListType = KM_TRANS_POLYGON;
	TextContext.StripControl.nUserClipMode = KM_USERCLIP_DISABLE;
	TextContext.StripControl.nShadowMode = KM_NORMAL_POLYGON;
	TextContext.StripControl.bOffset = KM_FALSE;
	TextContext.StripControl.bGouraud = KM_TRUE;
	TextContext.ObjectControl.nDepthCompare = KM_ALWAYS;
	TextContext.ObjectControl.nCullingMode = KM_NOCULLING;
	TextContext.ObjectControl.bZWriteDisable = KM_FALSE;
	TextContext.ObjectControl.bDCalcControl = KM_FALSE;
	BaseColour.dwPacked = 0xFF00FF00;
	TextContext.type.splite.Base = BaseColour;
	TextContext.ImageControl[ KM_IMAGE_PARAM1 ].nSRCBlendingMode = KM_SRCALPHA;
	TextContext.ImageControl[ KM_IMAGE_PARAM1 ].nDSTBlendingMode =
		KM_INVSRCALPHA;
	TextContext.ImageControl[ KM_IMAGE_PARAM1 ].bSRCSelect = KM_FALSE;
	TextContext.ImageControl[ KM_IMAGE_PARAM1 ].bDSTSelect = KM_FALSE;
	TextContext.ImageControl[ KM_IMAGE_PARAM1 ].nFogMode = KM_NOFOG;
	TextContext.ImageControl[ KM_IMAGE_PARAM1 ].bColorClamp = KM_FALSE;
	TextContext.ImageControl[ KM_IMAGE_PARAM1 ].bUseAlpha = KM_TRUE;
	TextContext.ImageControl[ KM_IMAGE_PARAM1 ].bIgnoreTextureAlpha = KM_FALSE;
	TextContext.ImageControl[ KM_IMAGE_PARAM1 ].nFlipUV = KM_NOFLIP;
	TextContext.ImageControl[ KM_IMAGE_PARAM1 ].nClampUV = KM_CLAMP_UV;
	TextContext.ImageControl[ KM_IMAGE_PARAM1 ].nFilterMode = KM_BILINEAR;
	TextContext.ImageControl[ KM_IMAGE_PARAM1 ].bSuperSampleMode = KM_FALSE;
	TextContext.ImageControl[ KM_IMAGE_PARAM1 ].dwMipmapAdjust = 
		KM_MIPMAP_D_ADJUST_1_00;
	TextContext.ImageControl[ KM_IMAGE_PARAM1 ].nTextureShadingMode =
		KM_MODULATE;
	TextContext.ImageControl[ KM_IMAGE_PARAM1 ].pTextureSurfaceDesc = NULL;

	kmGenerateStripHead16( &TEX_StripHead, &TextContext );

	return 0;
}

int TEX_CreateGlyphSetFromFile( char *p_pFileName, GLYPHSET *p_pGlyphSet )
{
	GDFS FileHandle;
	long FileBlocks;
	char *pFileContents;
	char *pLine;
	char Token[ 256 ];
	char *pLineChar;
	size_t FilePosition = 0;
	BMF_CHUNK Chunk;
	Sint32 FileSize;

	if( !( FileHandle = FS_OpenFile( p_pFileName ) ) )
	{
		LOG_Debug( "Failed to open glyph file" );

		return 1;
	}

	gdFsGetFileSize( FileHandle, &FileSize );

	gdFsGetFileSctSize( FileHandle, &FileBlocks );

	pFileContents = syMalloc( FileBlocks * 2048 );

	if( gdFsReqRd32( FileHandle, FileBlocks, pFileContents ) < 0 )
	{
		LOG_Debug( "Could not load the glyph file into memory" );

		return 1;
	}

	while( gdFsGetStat( FileHandle ) != GDD_STAT_COMPLETE )
	{
	}

	gdFsClose( FileHandle );

	if( ( pFileContents[ 0 ] != 'B' ) ||
		( pFileContents[ 1 ] != 'M' ) ||
		( pFileContents[ 2 ] != 'F' ) ||
		( pFileContents[ 3 ] != 0x03 ) )
	{
		syFree( pFileContents );

		LOG_Debug( "Font file is not a binary file" );

		return 1;
	}

	FilePosition = 4;

	while( FilePosition != FileSize )
	{
		size_t TestSize = sizeof( BMF_CHAR );
		Sint32 Size = 0;
		memcpy( &Chunk, &pFileContents[ FilePosition ], sizeof( BMF_CHUNK ) );
		/* Very ugly code */
		Size = ( Chunk.Size[ 0 ] );
		Size |= ( Chunk.Size[ 1 ] << 8 );
		Size |= ( ( Chunk.Size[ 2 ] ) << 16 );
		Size |= ( ( Chunk.Size[ 3 ] ) << 24 );

		FilePosition += sizeof( BMF_CHUNK );

		switch( Chunk.ID )
		{
			case BMF_ID_COMMON:
			{
				BMF_COMMON Common;

				memcpy( &Common, &pFileContents[ FilePosition ],
					BMF_COMMON_PACKED_SIZE );

				FilePosition += BMF_COMMON_PACKED_SIZE;

				p_pGlyphSet->LineHeight = Common.LineHeight;
				p_pGlyphSet->BaseLine = Common.BaseLine;
				p_pGlyphSet->Width = Common.Width;
				p_pGlyphSet->Height = Common.Height;

				break;
			}
			case BMF_ID_CHARS:
			{
				size_t CharCount = Size / sizeof( BMF_CHAR );
				size_t CharIndex;

				for( CharIndex = 0; CharIndex < CharCount; ++CharIndex )
				{
					BMF_CHAR Char;

					memcpy( &Char, &pFileContents[ FilePosition ],
						sizeof( BMF_CHAR ) );

					p_pGlyphSet->Glyphs[ Char.ID ].X = Char.X;
					p_pGlyphSet->Glyphs[ Char.ID ].Y = Char.Y;
					p_pGlyphSet->Glyphs[ Char.ID ].Width = Char.Width;
					p_pGlyphSet->Glyphs[ Char.ID ].Height = Char.Height;
					p_pGlyphSet->Glyphs[ Char.ID ].XOffset = Char.XOffset;
					p_pGlyphSet->Glyphs[ Char.ID ].YOffset = Char.YOffset;
					p_pGlyphSet->Glyphs[ Char.ID ].XAdvance = Char.XAdvance;

					FilePosition += sizeof( BMF_CHAR );
				}
				break;
			}
			default:
			{
				FilePosition += Size;
				break;
			}
		}
	}

	syFree( pFileContents );

	return 0;
}

int TEX_SetTextureForGlyphSet( char *p_pFileName, GLYPHSET *p_pGlyphSet )
{
	GDFS FileHandle;
	long FileBlocks;
	PKMDWORD pTexture;

	if( !( FileHandle = FS_OpenFile( p_pFileName ) ) )
	{
		LOG_Debug( "Failed to open texture for the glyph set" );

		return 1;
	}

	gdFsGetFileSctSize( FileHandle, &FileBlocks );
	pTexture = syMalloc( FileBlocks * 2048 );

	if( gdFsReqRd32( FileHandle, FileBlocks, pTexture ) < 0 )
	{
		LOG_Debug( "Failed to load the glyph set texture into memory" );

		return 1;
	}

	while( gdFsGetStat( FileHandle ) != GDD_STAT_COMPLETE )
	{
	}

	gdFsClose( FileHandle );

	kmCreateTextureSurface( &p_pGlyphSet->Texture, p_pGlyphSet->Width,
		p_pGlyphSet->Height,
		( KM_TEXTURE_TWIDDLED | KM_TEXTURE_ARGB4444 ) );

	/* Add 16 bytes to skip the PVRT header */
	if( kmLoadTexture( &( p_pGlyphSet )->Texture, pTexture + 4 ) !=
		KMSTATUS_SUCCESS )
	{
		syFree( pTexture );

		LOG_Debug( "Failed to load the glyph texture into video memory" );

		return 1;
	}

	syFree( pTexture );

	return 0;
}

void TEX_RenderString( GLYPHSET *p_pGlyphSet, float p_X, float p_Y,
	char *p_pString )
{
	size_t StringLength;
	size_t Char;
	float XPos = p_X, YPos = p_Y;
	union
	{
		KMFLOAT F[ 2 ];
		KMWORD	F16[ 4 ];
	}F;
	size_t IndexChar;

	StringLength = strlen( p_pString );

	kmChangeStripTextureSurface( &TEX_StripHead, KM_IMAGE_PARAM1,
		&( p_pGlyphSet )->Texture );

	for( Char = 0; Char < StringLength - 1; ++Char )
	{
		IndexChar = p_pString[ Char ] < 0 ?
			( p_pString[ Char ] & 0x7F ) + 128 : p_pString[ Char ];

		TEX_TextBuffer[ Char ].ParamControlWord = KM_VERTEXPARAM_NORMAL;
		TEX_TextBuffer[ Char ].fAX = XPos +
			( float )p_pGlyphSet->Glyphs[ IndexChar ].XOffset;
		TEX_TextBuffer[ Char ].fAY = YPos +
			( float )p_pGlyphSet->Glyphs[ IndexChar ].YOffset;
		TEX_TextBuffer[ Char ].uA.fAZ = 256.0f;
		TEX_TextBuffer[ Char ].fBX = XPos +
			( float )p_pGlyphSet->Glyphs[ IndexChar ].Width +
			( float )p_pGlyphSet->Glyphs[ IndexChar ].XOffset;
		TEX_TextBuffer[ Char ].fBY = YPos +
			( float )p_pGlyphSet->Glyphs[ IndexChar ].YOffset;
		TEX_TextBuffer[ Char ].uB.fBZ = 256.0f;
		TEX_TextBuffer[ Char ].fCX = XPos +
			( float )p_pGlyphSet->Glyphs[ IndexChar ].Width +
			( float )p_pGlyphSet->Glyphs[ IndexChar ].XOffset;
		TEX_TextBuffer[ Char ].fCY = YPos +
			( float )p_pGlyphSet->Glyphs[ IndexChar ].Height +
			( float )p_pGlyphSet->Glyphs[ IndexChar ].YOffset;
		TEX_TextBuffer[ Char ].uC.fCZ = 256.0f;
		TEX_TextBuffer[ Char ].fDX = XPos +
			( float )p_pGlyphSet->Glyphs[ IndexChar ].XOffset;
		TEX_TextBuffer[ Char ].fDY = YPos +
			( float )p_pGlyphSet->Glyphs[ IndexChar ].Height +
			( float )p_pGlyphSet->Glyphs[ IndexChar ].YOffset;

		F.F[ 1 ] = ( float )p_pGlyphSet->Glyphs[ IndexChar ].X /
			( float )p_pGlyphSet->Width;
		F.F[ 0 ] = ( float )p_pGlyphSet->Glyphs[ IndexChar ].Y /
			( float )p_pGlyphSet->Height;
		TEX_TextBuffer[ Char ].dwUVA = ( ( KMDWORD )F.F16[ 3 ] << 16 ) |
			( KMDWORD )F.F16[ 1 ];

		F.F[ 1 ] = ( ( float )p_pGlyphSet->Glyphs[ IndexChar ].X +
			( float )p_pGlyphSet->Glyphs[ IndexChar ].Width ) /
			( float )p_pGlyphSet->Width;
		F.F[ 0 ] = ( float )p_pGlyphSet->Glyphs[ IndexChar ].Y /
			( float )p_pGlyphSet->Height;
		TEX_TextBuffer[ Char ].dwUVB = ( ( KMDWORD )F.F16[ 3 ] << 16 ) |
			( KMDWORD )F.F16[ 1 ];

		F.F[ 1 ] = ( ( float )p_pGlyphSet->Glyphs[ IndexChar ].X +
			( float )p_pGlyphSet->Glyphs[ IndexChar ].Width ) /
			( float )p_pGlyphSet->Width;
		F.F[ 0 ] = ( ( float )p_pGlyphSet->Glyphs[ IndexChar ].Y +
			( float )p_pGlyphSet->Glyphs[ IndexChar ].Height ) /
			( float )p_pGlyphSet->Height;
		TEX_TextBuffer[ Char ].dwUVC = ( ( KMDWORD )F.F16[ 3 ] << 16 ) |
			( KMDWORD )F.F16[ 1 ];

		XPos += ( float )p_pGlyphSet->Glyphs[ IndexChar ].XAdvance;
	}

	IndexChar = p_pString[ Char ] < 0 ? p_pString[ Char ] + 128 :
		p_pString[ Char ];

	TEX_TextBuffer[ Char ].ParamControlWord = KM_VERTEXPARAM_ENDOFSTRIP;
	TEX_TextBuffer[ Char ].fAX = XPos +
		( float )p_pGlyphSet->Glyphs[ IndexChar ].XOffset;
	TEX_TextBuffer[ Char ].fAY = YPos +
		( float )p_pGlyphSet->Glyphs[ IndexChar ].YOffset;
	TEX_TextBuffer[ Char ].uA.fAZ = 256.0f;
	TEX_TextBuffer[ Char ].fBX = XPos +
		( float )p_pGlyphSet->Glyphs[ IndexChar ].Width +
		( float )p_pGlyphSet->Glyphs[ IndexChar ].XOffset;
	TEX_TextBuffer[ Char ].fBY = YPos +
		( float )p_pGlyphSet->Glyphs[ IndexChar ].YOffset;
	TEX_TextBuffer[ Char ].uB.fBZ = 256.0f;
	TEX_TextBuffer[ Char ].fCX = XPos +
		( float )p_pGlyphSet->Glyphs[ IndexChar ].Width +
		( float )p_pGlyphSet->Glyphs[ IndexChar ].XOffset;
	TEX_TextBuffer[ Char ].fCY = YPos +
		( float )p_pGlyphSet->Glyphs[ IndexChar ].Height +
		( float )p_pGlyphSet->Glyphs[ IndexChar ].YOffset;
	TEX_TextBuffer[ Char ].uC.fCZ = 256.0f;
	TEX_TextBuffer[ Char ].fDX = XPos +
		( float )p_pGlyphSet->Glyphs[ IndexChar ].XOffset;
	TEX_TextBuffer[ Char ].fDY = YPos +
		( float )p_pGlyphSet->Glyphs[ IndexChar ].Height +
		( float )p_pGlyphSet->Glyphs[ IndexChar ].YOffset;

	F.F[ 1 ] = ( float )p_pGlyphSet->Glyphs[ IndexChar ].X /
		( float )p_pGlyphSet->Width;
	F.F[ 0 ] = ( float )p_pGlyphSet->Glyphs[ IndexChar ].Y /
		( float )p_pGlyphSet->Height;
	TEX_TextBuffer[ Char ].dwUVA = ( ( KMDWORD )F.F16[ 3 ] << 16 ) |
		( KMDWORD )F.F16[ 1 ];

	F.F[ 1 ] = ( ( float )p_pGlyphSet->Glyphs[ IndexChar ].X +
		( float )p_pGlyphSet->Glyphs[ IndexChar ].Width ) /
		( float )p_pGlyphSet->Width;
	F.F[ 0 ] = ( float )p_pGlyphSet->Glyphs[ IndexChar ].Y /
		( float )p_pGlyphSet->Height;
	TEX_TextBuffer[ Char ].dwUVB = ( ( KMDWORD )F.F16[ 3 ] << 16 ) |
		( KMDWORD )F.F16[ 1 ];

	F.F[ 1 ] = ( ( float )p_pGlyphSet->Glyphs[ IndexChar ].X +
		( float )p_pGlyphSet->Glyphs[ IndexChar ].Width ) /
		( float )p_pGlyphSet->Width;
	F.F[ 0 ] = ( ( float )p_pGlyphSet->Glyphs[ IndexChar ].Y +
		( float )p_pGlyphSet->Glyphs[ IndexChar ].Height ) /
		( float )p_pGlyphSet->Height;
	TEX_TextBuffer[ Char ].dwUVC = ( ( KMDWORD )F.F16[ 3 ] << 16 ) |
		( KMDWORD )F.F16[ 1 ];

	REN_DrawPrimitives16( &TEX_StripHead, TEX_TextBuffer, StringLength );
}

void TEX_MeasureString( GLYPHSET *p_pGlyphSet, char *p_pString,
	float *p_pWidth )
{
	size_t StringLength;
	size_t Char;

	( *p_pWidth ) = 0.0f;

	StringLength = strlen( p_pString );

	for( Char = 0; Char < StringLength; ++Char )
	{
		size_t IndexChar = p_pString[ Char ] < 0 ? p_pString[ Char ] + 128 :
			p_pString[ Char ];

		( *p_pWidth ) +=
			( float )p_pGlyphSet->Glyphs[ IndexChar ].XAdvance;
	}
}
