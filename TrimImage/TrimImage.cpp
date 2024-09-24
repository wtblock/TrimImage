/////////////////////////////////////////////////////////////////////////////
// Copyright © by W. T. Block, all rights reserved
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TrimImage.h"
#include "CHelper.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
// The one and only application object
CWinApp theApp;

/////////////////////////////////////////////////////////////////////////////
// Save the data inside pImage to the given filename but relocated to the 
// sub-folder "Corrected"
bool Save( LPCTSTR lpszPathName, Gdiplus::Bitmap* pImage )
{
	USES_CONVERSION;

	// save and overwrite the selected image file with current page
	int iValue =
		Gdiplus::EncoderValue::EncoderValueVersionGif89 |
		Gdiplus::EncoderValue::EncoderValueCompressionLZW |
		Gdiplus::EncoderValue::EncoderValueFlush;

	Gdiplus::EncoderParameters param;
	param.Count = 1;
	param.Parameter[ 0 ].Guid = Gdiplus::EncoderSaveFlag;
	param.Parameter[ 0 ].Value = &iValue;
	param.Parameter[ 0 ].Type = Gdiplus::EncoderParameterValueTypeLong;
	param.Parameter[ 0 ].NumberOfValues = 1;

	// writing to the same file will fail, so save to a corrected folder
	// below the image being corrected
	const CString csCorrected = GetCorrectedFolder();
	const CString csFolder = CHelper::GetFolder( lpszPathName ) + csCorrected;
	if ( !::PathFileExists( csFolder ) )
	{
		if ( !CreatePath( csFolder ) )
		{
			return false;
		}
	}

	// filename plus extension
	const CString csData = CHelper::GetDataName( lpszPathName );

	// create a new path from the pieces
	const CString csPath = csFolder + _T( "\\" ) + csData;

	// use the extension member class to get the class ID of the file
	CLSID clsid = m_Extension.ClassID;

	// set the horizontal and vertical DPI to match the original image
	pImage->SetResolution( m_fHorizontalResolution, m_fVerticalResolution );

	// save the image to the corrected folder
	Status status = pImage->Save( T2CW( csPath ), &clsid, &param );

	// return true if the save worked
	return status == Ok;
} // Save

/////////////////////////////////////////////////////////////////////////////
// handle an aspect ratio change if requested by modifying the new image
// dimensions
bool HandleAspectRatio()
{
	bool value = false;

	// calculate the new width
	m_uiNewWidth = m_uiOriginalWidth - m_uiLeft - m_uiRight;

	// calculate the new height
	m_uiNewHeight = m_uiOriginalHeight - m_uiTop - m_uiBottom;

	// did the user request a fixed aspect ratio?
	bool bAspect = GetProcessAspect();

	// factor in the requested aspect ratio if requested
	const float fRequestedRatio = GetRequestedAspectRatio();

	// aspect ratio of the original file
	const float fOriginalRatio = GetOriginalAspectRatio();

	if ( bAspect )
	{
		// test for valid ratio value
		if ( CHelper::NearlyEqual( fRequestedRatio, 0.0f ) )
		{
			bAspect = false;
		} 
		else if ( CHelper::NearlyEqual( fRequestedRatio, fOriginalRatio ) )
		{
			bAspect = false;
		}
	}

	if ( bAspect )
	{
		value = true;

		// update the height, top and bottom values
		if ( fRequestedRatio > fOriginalRatio )
		{
			// calculate new height based on the aspect ratio
			// r = w / h
			// h = w / r
			m_uiNewHeight = UINT( float( m_uiOriginalWidth ) / fRequestedRatio );
			const UINT uiDelta = m_uiOriginalHeight - m_uiNewHeight;
			m_uiTop = uiDelta / 2;
			m_uiBottom = m_uiTop;
		} 
		else // update the width, left and right values
		{
			// calculate new width based on the aspect ratio
			// r = w / h
			// w = r * h
			m_uiNewWidth = UINT( float( m_uiOriginalHeight ) * fRequestedRatio );
			const UINT uiDelta = m_uiOriginalWidth - m_uiNewWidth;
			m_uiLeft = uiDelta / 2;
			m_uiRight = m_uiLeft;
		}
	}

	return value;
} // HandleAspectRatio

/////////////////////////////////////////////////////////////////////////////
// modify the image to reflect the user command line parameter
bool ProcessImage( CString& csPath, CStdioFile& fout )
{
	bool value = false;

	// preserve the original values from the command line parameters
	const UINT uiTop = m_uiTop;
	const UINT uiBottom = m_uiBottom;
	const UINT uiLeft = m_uiLeft;
	const UINT uiRight = m_uiRight;

	// valid file extensions
	const CString csValidExt = _T( ".jpg;.jpeg;.png;.gif;.bmp;.tif;.tiff" );

	// the file extension of the current file
	const CString csExt = CHelper::GetExtension( csPath ).MakeLower();

	// the name of the current file without the extension
	const CString csFile = CHelper::GetFileName( csPath );

	// test to see if the extension is one we support
	if ( -1 != csValidExt.Find( csExt ) )
	{
		// set the extension property
		m_Extension.FileExtension = csExt;

		// let the user know the file being processed
		fout.WriteString( csPath + _T( "\n" ) );

		// image representing this file
		Gdiplus::Image OriginalImage( T2CW( csPath ) );

		// get the width of the image
		m_uiOriginalWidth = OriginalImage.GetWidth();

		// get the height of the image
		m_uiOriginalHeight = OriginalImage.GetHeight();

		// remember the resolution (DPI) of the original image so the
		// generated image can be set to the same resolution
		m_fHorizontalResolution = OriginalImage.GetHorizontalResolution();
		m_fVerticalResolution = OriginalImage.GetVerticalResolution();

		// get the new width based on trimming parameters
		m_uiNewWidth = m_uiOriginalWidth - m_uiLeft - m_uiRight;

		// get the new width based on trimming parameters
		m_uiNewHeight = m_uiOriginalHeight - m_uiLeft - m_uiRight;

		// if the user specified an aspect ratio, other parameters
		// like top and bottom or left and right can be modified
		const bool bAspect = HandleAspectRatio();

		// let the user know what is going on
		CString csOutput;
		csOutput.Format
		(
			_T( "Org Dimensions: %d, %d\n" ),
			m_uiOriginalHeight, m_uiOriginalWidth
		);
		fout.WriteString( csOutput );

		csOutput.Format
		(
			_T( "New Dimensions: %d, %d\n" ),
			m_uiNewHeight, m_uiNewWidth
		);
		fout.WriteString( csOutput );

		// Create a new bitmap with the trimmed dimensions
		Gdiplus::Bitmap trimmedBitmap( m_uiNewWidth, m_uiNewHeight );

		// create a graphics object to draw the new bitmap
		Gdiplus::Graphics graphics( &trimmedBitmap );

		// draw the original image into the new image
		graphics.DrawImage
		(
			&OriginalImage, Gdiplus::Rect( 0, 0, m_uiNewWidth, m_uiNewHeight ),
			m_uiLeft, m_uiTop, m_uiNewWidth, m_uiNewHeight, Gdiplus::UnitPixel
		);

		// save the image to the new path
		value = Save( csPath, &trimmedBitmap );
	}

	// restore the original values of the command line parameters
	m_uiTop = uiTop;
	m_uiBottom = uiBottom;
	m_uiLeft = uiLeft;
	m_uiRight = uiRight;

	return value;
} // ProcessImage

/////////////////////////////////////////////////////////////////////////////
// crawl through the directory tree looking for supported image extensions
void RecursePath( LPCTSTR path, CStdioFile& fout )
{
	USES_CONVERSION;

	// the new folder under the image folder to contain the corrected images
	const CString csCorrected = GetCorrectedFolder();
	const int nCorrected = GetCorrectedFolderLength();

	// get the folder which will trim any wild card data
	CString csPathname = CHelper::GetFolder( path );

	// wild cards are in use if the pathname does not equal the given path
	const bool bWildCards = csPathname != path;
	csPathname.TrimRight( _T( "\\" ) );
	CString csData;

	// build a string with wild-cards
	CString strWildcard;
	if ( bWildCards )
	{
		csData = CHelper::GetDataName( path );
		strWildcard.Format( _T( "%s\\%s" ), csPathname, csData );

	} else // no wild cards, just a folder
	{
		strWildcard.Format( _T( "%s\\*.*" ), csPathname );
	}

	// start trolling for files we are interested in
	CFileFind finder;
	BOOL bWorking = finder.FindFile( strWildcard );
	while ( bWorking )
	{
		bWorking = finder.FindNextFile();

		// skip "." and ".." folder names
		if ( finder.IsDots() )
		{
			continue;
		}

		// if it's a directory, recursively search it
		if ( finder.IsDirectory() )
		{
			// do not recurse into the corrected folder
			const CString str = 
				finder.GetFilePath().TrimRight( _T( "\\" ) );
			if ( str.Right( nCorrected ) == csCorrected )
			{
				continue;
			}

			// if wild cards are in use, build a path with the wild cards
			if ( bWildCards )
			{
				CString csPath;
				csPath.Format( _T( "%s\\%s" ), str, csData );

				// recurse into the new directory with wild cards
				RecursePath( csPath, fout );

			} else // recurse into the new directory
			{
				RecursePath( str + _T( "\\" ), fout );
			}

		} else // write the properties if it is a valid extension
		{
			// the pathname of the current file
			CString csPath = finder.GetFilePath();

			// process the current file if it is a valid image
			const bool bOkay = ProcessImage( csPath, fout );
			if ( bOkay == false )
			{
				CString csOutput;
				csOutput.Format( _T( "Image save failed:\n\t%s\n" ), csPath );
				fout.WriteString( csOutput );
			}
		}
	}

	finder.Close();

} // RecursePath

/////////////////////////////////////////////////////////////////////////////
// set the current file extension which will automatically lookup the
// related mime type and class ID and set their respective properties
void CExtension::SetFileExtension( CString value )
{
	USES_CONVERSION;

	m_csFileExtension = value;

	if ( m_mapExtensions.Exists[ value ] )
	{
		MimeType = *m_mapExtensions.find( value );

		// populate the mime type map the first time it is referenced
		if ( m_mapMimeTypes.Count == 0 )
		{
			UINT num = 0;
			UINT size = 0;

			// gets the number of available image encoders and 
			// the total size of the array
			Gdiplus::GetImageEncodersSize( &num, &size );
			if ( size == 0 )
			{
				return;
			}

			Gdiplus::ImageCodecInfo* pImageCodecInfo =
				(Gdiplus::ImageCodecInfo*)malloc( size );
			if ( pImageCodecInfo == nullptr )
			{
				return;
			}

			// Returns an array of ImageCodecInfo objects that contain 
			// information about the image encoders built into GDI+.
			Gdiplus::GetImageEncoders( num, size, pImageCodecInfo );

			// populate the map of mime types the first time it is 
			// needed
			for ( UINT nIndex = 0; nIndex < num; ++nIndex )
			{
				CString csKey;
				csKey = CW2A( pImageCodecInfo[ nIndex ].MimeType );
				CLSID classID = pImageCodecInfo[ nIndex ].Clsid;
				m_mapMimeTypes.add( csKey, new CLSID( classID ) );
			}

			// clean up
			free( pImageCodecInfo );
		}

		ClassID = *m_mapMimeTypes.find( MimeType );

	} else
	{
		MimeType = _T( "" );
	}
} // CExtension::SetFileExtension

/////////////////////////////////////////////////////////////////////////////
// give the user some usage help if the parameters do not look right
void Usage( CStdioFile& fOut )
{
	fOut.WriteString( _T( ".\n" ) );
	fOut.WriteString
	(
		_T( "TrimImage, Copyright (c) 2024, " )
		_T( "by W. T. Block.\n" )
	);

	fOut.WriteString
	(
		_T( ".\n" )
		_T( "A Windows command line program to trim image(s) using\n" )
		_T( "  the given the number of pixels to be trimmed from\n" )
		_T( "  each of the sides (top, bottom, left, and right) or\n" )
		_T( "  to change the aspect ratio (example a=3:2).\n" )
		_T( ".\n" )
	);

	fOut.WriteString
	(
		_T( ".\n" )
		_T( "Usage:\n" )
		_T( ".\n" )
		_T( ".  TrimImage pathname [t=top b=bottom l=left r=right a=aspect]\n" )
		_T( ".\n" )
		_T( "Where:\n" )
		_T( ".\n" )
	);

	fOut.WriteString
	(
		_T( ".  pathname is the root of the tree to be scanned, but\n" )
		_T( ".  may contain wild cards like the following:\n" )
		_T( ".    \"c:\\Picture\\DisneyWorldMary2\\*.JPG\"\n" )
		_T( ".  will process all files with that pattern, or\n" )
		_T( ".    \"c:\\Picture\\DisneyWorldMary2\\231.JPG\"\n" )
		_T( ".  will process a single defined image file, or\n" )
		_T( ".    \"c:\\Picture\\DisneyWorldMary2\\\"\n" )
		_T( ".  will recurse the folder and its sub-folders.\n" )
		_T( ".  (NOTE: using wild cards will prevent recursion\n" )
		_T( ".    into sub-folders because the folders will likely\n" )
		_T( ".    not fall into the same pattern and therefore\n" )
		_T( ".    sub-folders will not be found by the search).\n" )
	);

	fOut.WriteString
	(
		_T( ".  all parameters are optional.\n" )
		_T( ".  top is the number of pixels trimmed from the top.\n" )
		_T( ".  bottom is the number of pixels trimmed from the bottom.\n" )
		_T( ".  left is the number of pixels trimmed from the left.\n" )
		_T( ".  right is the number of pixels trimmed from the right.\n" )
		_T( ".  aspect is the aspect ratio in the form of 'width:height'\n" )
		_T( ".\n" )
		_T( ".Examples: \n" )
		_T( ".  TrimImage . a=3:2\n" )
		_T( ".    will force the aspect ratio to become 3:2\n" )
		_T( ".  TrimImage . t=40 l=5\n" )
		_T( ".    will trim 40 pixels from the top of the image\n" )
		_T( ".    and 5 pixels from the left of the image.\n" )
		_T( ".\n" )
		_T( ".NOTE: \n" )
		_T( ".  Trimming parameters can all be set at once, but\n" )
		_T( ".  changing the aspect ratio should be done alone.\n" )
		_T( ".  Aspect ratio causes its own trimming changes\n" )
		_T( ".  and mixing the operations is unpredictable.\n" )
		_T( ".\n" )
	);
} // Usage

/////////////////////////////////////////////////////////////////////////////
// a console application that can crawl through the file
// system and troll for image metadata properties
int _tmain( int argc, TCHAR* argv[], TCHAR* envp[] )
{
	HMODULE hModule = ::GetModuleHandle( NULL );
	if ( hModule == NULL )
	{
		_tprintf( _T( "Fatal Error: GetModuleHandle failed\n" ) );
		return 1;
	}

	// initialize MFC and error on failure
	if ( !AfxWinInit( hModule, NULL, ::GetCommandLine(), 0 ) )
	{
		_tprintf( _T( "Fatal Error: MFC initialization failed\n " ) );
		return 2;
	}

	// do some common command line argument corrections
	vector<CString> arrArgs = CHelper::CorrectedCommandLine( argc, argv );
	size_t nArgs = arrArgs.size();

	CStdioFile fOut( stdout );
	CString csMessage;

	// display the number of arguments if not 1 to help the user 
	// understand what went wrong if there is an error in the
	// command line syntax
	if ( nArgs != 1 )
	{
		fOut.WriteString( _T( ".\n" ) );
		csMessage.Format( _T( "The number of parameters are %d\n.\n" ), nArgs - 1 );
		fOut.WriteString( csMessage );

		// display the arguments
		for ( int i = 1; i < nArgs; i++ )
		{
			csMessage.Format( _T( "Parameter %d is %s\n.\n" ), i, arrArgs[ i ] );
			fOut.WriteString( csMessage );
		}
	}

	// five arguments if specifying year, month, and day
	// two arguments if using the existing date taken
	if ( nArgs < 3 || nArgs > 7 )
	{
		Usage( fOut );
		return 3;
	}

	// display the executable path
	//csMessage.Format( _T( "Executable pathname: %s\n" ), arrArgs[ 0 ] );
	//fOut.WriteString( _T( ".\n" ) );
	//fOut.WriteString( csMessage );
	//fOut.WriteString( _T( ".\n" ) );

	// retrieve the pathname which may include wild cards
	CString csPath = arrArgs[ 1 ];

	// trim off any wild card data
	const CString csFolder = CHelper::GetFolder( csPath );

	// test for current folder character (a period)
	bool bExists = csPath == _T( "." );

	// if it is a period, add a wild card of *.* to retrieve
	// all folders and files
	if ( bExists )
	{
		csPath = _T( ".\\*.*" );

		// if it is not a period, test to see if the folder exists
	}
	else
	{
		if ( ::PathFileExists( csFolder ) )
		{
			bExists = true;
		}
	}

	if ( !bExists )
	{
		csMessage.Format( _T( "Invalid pathname:\n\t%s\n" ), csPath );
		fOut.WriteString( _T( ".\n" ) );
		fOut.WriteString( csMessage );
		fOut.WriteString( _T( ".\n" ) );
		return 4;

	}
	else
	{
		csMessage.Format( _T( "Given pathname:\n\t%s\n" ), csPath );
		fOut.WriteString( _T( ".\n" ) );
		fOut.WriteString( csMessage );
	}

	// initialize the command line parameters to their default values
	m_uiTop = 0;
	m_uiBottom = 0;
	m_uiLeft = 0;
	m_uiRight = 0;

	// initialize intermediate values
	m_uiOriginalWidth = 1;
	m_uiOriginalHeight = 1;
	m_fHorizontalResolution = 600.0f;
	m_fVerticalResolution = 600.0f;
	m_uiNewWidth = 1;
	m_uiNewHeight = 1;
	m_uiAspectWidth = 1;
	m_uiAspectHeight = 1;

	// parse the command line arguments
	CString csArg;
	for ( int nArg = 2; nArg < nArgs; nArg++ )
	{
		csArg = arrArgs[ nArg ].MakeLower();
		int nStart = 0;
		CString csOp, csValue;
		csOp = csArg.Tokenize( _T( "=" ), nStart );
		if ( csOp.IsEmpty() )
		{
			continue;
		}
		csValue = csArg.Tokenize( _T( "=" ), nStart );
		if ( csOp == _T( "t" ) )
		{
			m_uiTop = _tstol( csValue );

		} else if ( csOp == _T( "b" ) )
		{
			m_uiBottom = _tstol( csValue );

		} else if ( csOp == _T( "l" ) )
		{
			m_uiLeft = _tstol( csValue );

		} else if ( csOp == _T( "r" ) )
		{
			m_uiRight = _tstol( csValue );

		} else if ( csOp == _T( "a" ) )
		{
			m_csAspect = csValue;

		} else
		{
			Usage( fOut );
			return 5;
		}

	}

	// start up COM
	AfxOleInit();
	::CoInitialize( NULL );

	// create a reference to GDI+
	InitGdiplus();

	// crawl through directory tree defined by the command line
	// parameter trolling for supported image files
	RecursePath( csPath, fOut );

	// clean up references to GDI+
	TerminateGdiplus();

	// all is good
	return 0;

} // _tmain
