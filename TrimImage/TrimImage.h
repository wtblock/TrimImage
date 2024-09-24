/////////////////////////////////////////////////////////////////////////////
// Copyright © by W. T. Block, all rights reserved
/////////////////////////////////////////////////////////////////////////////
#pragma once

#include "resource.h"
#include "KeyedCollection.h"
#include <vector>
#include <map>
#include <memory>
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;
using namespace std;

////////////////////////////////////////////////////////////////////////////
// this class creates a fast look up of the mime type and class ID as 
// defined by GDI+ for common file extensions
class CExtension
{
	// protected definitions
protected:
	typedef struct tagExtensionLookup
	{
		CString m_csFileExtension;
		CString m_csMimeType;

	} EXTENSION_LOOKUP;

	typedef struct tagClassLookup
	{
		CString m_csMimeType;
		CLSID m_ClassID;

	} CLASS_LOOKUP;

	// protected data
protected:
	// current file extension
	CString m_csFileExtension;

	// current mime type
	CString m_csMimeType;

	// current class ID
	CLSID m_ClassID;

	// cross reference of file extensions to mime types
	CKeyedCollection<CString, CString> m_mapExtensions;

	// cross reference of mime types to class IDs
	CKeyedCollection<CString, CLSID> m_mapMimeTypes;

	// public properties
public:
	// current file extension
	inline CString GetFileExtension()
	{
		return m_csFileExtension;
	}
	// current file extension
	void SetFileExtension( CString value );
	// current file extension
	__declspec( property( get = GetFileExtension, put = SetFileExtension ) )
		CString FileExtension;

	// image extension associated with the current file extension
	inline CString GetMimeType()
	{
		return m_csMimeType;
	}
	// image extension associated with the current file extension
	inline void SetMimeType( CString value )
	{
		m_csMimeType = value;
	}
	// get image extension associated with the current file extension
	__declspec( property( get = GetMimeType, put = SetMimeType ) )
		CString MimeType;

	// class ID associated with the current file extension
	inline CLSID GetClassID()
	{
		return m_ClassID;
	}
	// class ID associated with the current file extension
	inline void SetClassID( CLSID value )
	{
		m_ClassID = value;
	}
	// class ID associated with the current file extension
	__declspec( property( get = GetClassID, put = SetClassID ) )
		CLSID ClassID;

	// public methods
public:

	// protected methods
protected:

	// public virtual methods
public:

	// protected virtual methods
protected:

	// public construction
public:
	CExtension()
	{
		// extension conversion table
		static EXTENSION_LOOKUP ExtensionLookup[] =
		{
			{ _T( ".bmp" ), _T( "image/bmp" ) },
			{ _T( ".dib" ), _T( "image/bmp" ) },
			{ _T( ".rle" ), _T( "image/bmp" ) },
			{ _T( ".gif" ), _T( "image/gif" ) },
			{ _T( ".jpeg" ), _T( "image/jpeg" ) },
			{ _T( ".jpg" ), _T( "image/jpeg" ) },
			{ _T( ".jpe" ), _T( "image/jpeg" ) },
			{ _T( ".jfif" ), _T( "image/jpeg" ) },
			{ _T( ".png" ), _T( "image/png" ) },
			{ _T( ".tiff" ), _T( "image/tiff" ) },
			{ _T( ".tif" ), _T( "image/tiff" ) }
		};

		// build a cross reference of file extensions to 
		// mime types
		const int nPairs = _countof( ExtensionLookup );
		for ( int nPair = 0; nPair < nPairs; nPair++ )
		{
			const CString csKey =
				ExtensionLookup[ nPair ].m_csFileExtension;

			CString* pValue = new CString
			(
				ExtensionLookup[ nPair ].m_csMimeType
			);

			// add the pair to the collection
			m_mapExtensions.add( csKey, pValue );
		}
	}
};

/////////////////////////////////////////////////////////////////////////////
// used for gdiplus library
ULONG_PTR m_gdiplusToken;

/////////////////////////////////////////////////////////////////////////////
// this class creates a fast look up of the mime type and class ID as 
// defined by GDI+ for common file extensions
CExtension m_Extension;

/////////////////////////////////////////////////////////////////////////////
// pixels to trim from the top command line parameter
UINT m_uiTop;

/////////////////////////////////////////////////////////////////////////////
// pixels to trim from the bottom command line parameter
UINT m_uiBottom;

/////////////////////////////////////////////////////////////////////////////
// pixels to trim from the left command line parameter
UINT m_uiLeft;

/////////////////////////////////////////////////////////////////////////////
// pixels to trim from the right command line parameter
UINT m_uiRight;

/////////////////////////////////////////////////////////////////////////////
// aspect ratio command line parameter
CString m_csAspect;

/////////////////////////////////////////////////////////////////////////////
// aspect width command line parameter in the form of width:height
UINT m_uiAspectWidth;

/////////////////////////////////////////////////////////////////////////////
// aspect height command line parameter in the form of width:height
UINT m_uiAspectHeight;

// get the width of the image
UINT m_uiOriginalWidth;

// get the height of the image
UINT m_uiOriginalHeight;

// calculate the new width
UINT m_uiNewWidth;

// calculate the new height
UINT m_uiNewHeight;

/////////////////////////////////////////////////////////////////////////////
// calculate the aspect ratio given a width and height
// a value of zero indicates a failure
static inline float GetAspectRatio( UINT uiWidth, UINT uiHeight )
{
	float value = 0.0f;
	if ( uiHeight == 0 )
	{
		return value;
	}
	value = float( uiWidth ) / float( uiHeight );
	return value;
}

/////////////////////////////////////////////////////////////////////////////
// original image aspect ratio
// a value of zero indicates a failure
inline float GetOriginalAspectRatio()
{
	return GetAspectRatio( m_uiOriginalWidth, m_uiOriginalHeight );
}

/////////////////////////////////////////////////////////////////////////////
// new image aspect ratio
// a value of zero indicates a failure
inline float GetNewAspectRatio()
{
	return GetAspectRatio( m_uiNewWidth, m_uiNewHeight );
}

/////////////////////////////////////////////////////////////////////////////
// is the new image landscape mode (width larger than height)
inline bool GetLandscapeMode()
{
	bool value = m_uiOriginalWidth > m_uiOriginalHeight;
	return value;
}

/////////////////////////////////////////////////////////////////////////////
// did the user request an aspect change
inline bool GetProcessAspect()
{
	const bool value = !m_csAspect.IsEmpty();
	return value;
}

/////////////////////////////////////////////////////////////////////////////
// new image aspect ratio 
// a value of zero indicates a failure
inline float GetRequestedAspectRatio()
{
	float value = 0.0f;
	if ( GetProcessAspect() )
	{
		int nStart = 0;
		const CString csWidth = m_csAspect.Tokenize( _T( ":" ), nStart );
		if ( csWidth.IsEmpty() )
		{
			return value;
		}
		const CString csHeight = m_csAspect.Tokenize( _T( ":" ), nStart );
		if ( csHeight.IsEmpty() )
		{
			return value;
		}

		const bool bLandscape = GetLandscapeMode();

		const UINT uiWidth = _tstol( csWidth );
		const UINT uiHeight = _tstol( csHeight );

		// 3:2 remain 3:2 in landscape mode
		if ( bLandscape )
		{
			m_uiAspectWidth = max( uiWidth, uiHeight );
			m_uiAspectHeight = min( uiWidth, uiHeight );

		} else // 3:2 becomes 2:3 in portrait mode
		{
			m_uiAspectWidth = min( uiWidth, uiHeight );
			m_uiAspectHeight = max( uiWidth, uiHeight );
		}

		// calculate the aspect ratio from the command line parameters
		value = GetAspectRatio
		(
			m_uiAspectWidth,
			m_uiAspectHeight
		);
	}
	return value;
}

/////////////////////////////////////////////////////////////////////////////
// the new folder under the image folder to contain the corrected images
static inline CString GetCorrectedFolder()
{
	return _T( "Corrected" );
}

/////////////////////////////////////////////////////////////////////////////
// the new folder under the image folder to contain the corrected images
static inline int GetCorrectedFolderLength()
{
	const CString csFolder = GetCorrectedFolder();
	const int value = csFolder.GetLength();
	return value;
}

/////////////////////////////////////////////////////////////////////////////
// This function creates a file system folder whose fully qualified 
// path is given by pszPath. If one or more of the intermediate 
// folders do not exist, they will be created as well. 
// returns true if the path is created or already exists
bool CreatePath( LPCTSTR pszPath )
{
	if ( ERROR_SUCCESS == SHCreateDirectoryEx( NULL, pszPath, NULL ) )
	{
		return true;
	}

	return false;
} // CreatePath

/////////////////////////////////////////////////////////////////////////////
// initialize GDI+
bool InitGdiplus()
{
	GdiplusStartupInput gdiplusStartupInput;
	Status status = GdiplusStartup
	(
		&m_gdiplusToken,
		&gdiplusStartupInput,
		NULL
	);
	return ( Ok == status );
} // InitGdiplus

/////////////////////////////////////////////////////////////////////////////
// remove reference to GDI+
void TerminateGdiplus()
{
	GdiplusShutdown( m_gdiplusToken );
	m_gdiplusToken = NULL;

}// TerminateGdiplus

/////////////////////////////////////////////////////////////////////////////
