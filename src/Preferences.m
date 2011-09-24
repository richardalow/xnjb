//
//  Preferences.m
//  XNJB
//
//  Created by Richard Low on Tue Aug 31 2004.
//

/* Wrapper for preferences in NSUserDefaults
 * See keys/default objects in defs.h
 */

#import "Preferences.h"
#import "defs.h"

@implementation Preferences

// init/dealloc methods

- (id)init
{
	if (self = [super init])
	{
		defaults = [NSUserDefaults standardUserDefaults];
		[self registerDefaults];
	}
	return self;
}

// accessor methods

- (BOOL)connectOnStartup
{
	return [defaults boolForKey:PREF_KEY_CONNECT_ON_STARTUP];
}

- (void)setConnectOnStartup:(BOOL)connectOnStartup
{
	[defaults setObject:[NSNumber numberWithBool:connectOnStartup] forKey:PREF_KEY_CONNECT_ON_STARTUP];
}

- (int)startupTab
{
	int tab = [defaults integerForKey:PREF_KEY_STARTUP_TAB];
	if (tab >= 0)
		return tab;
	else
		return 0;
}

- (void)setStartupTab:(int)startupTab
{
	[defaults setObject:[NSNumber numberWithInt:startupTab] forKey:PREF_KEY_STARTUP_TAB];
}

- (BOOL)startupTabLastUsed
{
	return [defaults boolForKey:PREF_KEY_STARTUP_TAB_LAST_USED];
}

- (void)setStartupTabLastUsed:(BOOL)startupTabLastUsed
{
	[defaults setObject:[NSNumber numberWithBool:startupTabLastUsed] forKey:PREF_KEY_STARTUP_TAB_LAST_USED];
}

- (NSString *)musicTabDir
{
	return [defaults objectForKey:PREF_KEY_MUSIC_TAB_DIR];
}

- (void)setMusicTabDir:(NSString *)musicTabDir
{
	[defaults setObject:musicTabDir forKey:PREF_KEY_MUSIC_TAB_DIR];
}

- (NSString *)dataTabDir
{
	return [defaults objectForKey:PREF_KEY_DATA_TAB_DIR];
}

- (void)setDataTabDir:(NSString *)dataTabDir
{
	[defaults setObject:dataTabDir forKey:PREF_KEY_DATA_TAB_DIR];
}

- (BOOL)musicTabDirLastUsed
{
	return [defaults boolForKey:PREF_KEY_MUSIC_TAB_DIR_LAST_USED];
}

- (void)setMusicTabDirLastUsed:(BOOL)musicTabDirLastUsed
{
	[defaults setObject:[NSNumber numberWithBool:musicTabDirLastUsed] forKey:PREF_KEY_MUSIC_TAB_DIR_LAST_USED];
}

- (BOOL)dataTabDirLastUsed
{
	return [defaults boolForKey:PREF_KEY_DATA_TAB_DIR_LAST_USED];
}

- (void)setDataTabDirLastUsed:(BOOL)dataTabDirLastUsed
{
	[defaults setObject:[NSNumber numberWithBool:dataTabDirLastUsed] forKey:PREF_KEY_DATA_TAB_DIR_LAST_USED];
}

- (NSString *)filenameFormat
{
	return [defaults objectForKey:PREF_KEY_FILENAME_FORMAT];
}

- (void)setFilenameFormat:(NSString *)filenameFormat
{
	[defaults setObject:filenameFormat forKey:PREF_KEY_FILENAME_FORMAT];
}

- (BOOL)showDrawer
{
	return [defaults boolForKey:PREF_KEY_SHOW_DRAWER];
}

- (void)setShowDrawer:(BOOL)newShowDrawer
{
	[defaults setObject:[NSNumber numberWithBool:newShowDrawer] forKey:PREF_KEY_SHOW_DRAWER];
}

- (void)setLastUsedMusicTabDir:(NSString *)musicTabDir
{
	if ([self musicTabDirLastUsed])
		[self setMusicTabDir:musicTabDir];
}

- (void)setLastUsedDataTabDir:(NSString *)dataTabDir
{
	if ([self dataTabDirLastUsed])
		[self setDataTabDir:dataTabDir];
}

- (void)setLastUsedTab:(int)tab
{
	if ([self startupTabLastUsed])
		[self setStartupTab:tab];
}

- (void)setDrawerContentSize:(NSSize)size
{
	[defaults setObject:[NSNumber numberWithFloat:size.height] forKey:PREF_KEY_DRAWER_CONTENT_SIZE_HEIGHT];
	[defaults setObject:[NSNumber numberWithFloat:size.width] forKey:PREF_KEY_DRAWER_CONTENT_SIZE_WIDTH];
}

- (NSSize)drawerContentSize
{
	NSSize size;
	size.height = [defaults floatForKey:PREF_KEY_DRAWER_CONTENT_SIZE_HEIGHT];
	size.width = [defaults floatForKey:PREF_KEY_DRAWER_CONTENT_SIZE_WIDTH];
	return size;
}

- (BOOL)writeTagAfterCopy
{
	return [defaults boolForKey:PREF_KEY_WRITE_TAG_AFTER_COPY];
}

- (void)setWriteTagAfterCopy:(BOOL)writeTagAfterCopy
{
	[defaults setObject:[NSNumber numberWithBool:writeTagAfterCopy] forKey:PREF_KEY_WRITE_TAG_AFTER_COPY];
}

- (NSRectEdge)drawerEdge
{
	return [defaults integerForKey:PREF_KEY_DRAWER_EDGE];
}

- (void)setDrawerEdge:(NSRectEdge)drawerEdge
{
	[defaults setObject:[NSNumber numberWithInt:drawerEdge] forKey:PREF_KEY_DRAWER_EDGE];
}

- (int)lengthTol
{
	return [defaults integerForKey:PREF_KEY_DUPLICATES_LENGTH_TOL];
}

- (void)setLengthTol:(int)lengthTol
{
	[defaults setObject:[NSNumber numberWithInt:lengthTol] forKey:PREF_KEY_DUPLICATES_LENGTH_TOL];
}

- (int)filesizeTol
{
	return [defaults integerForKey:PREF_KEY_DUPLICATES_FILESIZE_TOL];
}

- (void)setFilesizeTol:(int)filesizeTol
{
	[defaults setObject:[NSNumber numberWithInt:filesizeTol] forKey:PREF_KEY_DUPLICATES_FILESIZE_TOL];
}

- (int)titleTol
{
	return [defaults integerForKey:PREF_KEY_DUPLICATES_TITLE_TOL];
}

- (void)setTitleTol:(int)titleTol
{
	[defaults setObject:[NSNumber numberWithInt:titleTol] forKey:PREF_KEY_DUPLICATES_TITLE_TOL];
}

- (int)artistTol
{
	return [defaults integerForKey:PREF_KEY_DUPLICATES_ARTIST_TOL];
}

- (void)setArtistTol:(int)artistTol
{
	[defaults setObject:[NSNumber numberWithInt:artistTol] forKey:PREF_KEY_DUPLICATES_ARTIST_TOL];
}

- (BOOL)turbo
{
	return [defaults boolForKey:PREF_KEY_TURBO];
}

- (void)setTurbo:(BOOL)turbo
{
	[defaults setObject:[NSNumber numberWithBool:turbo] forKey:PREF_KEY_TURBO];
	
	[theNJB setTurbo:turbo];
}

- (BOOL)iTunesIntegration
{
	return [defaults boolForKey:PREF_KEY_ITUNES_INTEGRATION];
}

- (void)setiTunesIntegration:(BOOL)iTunesIntegration
{
	[defaults setObject:[NSNumber numberWithBool:iTunesIntegration] forKey:PREF_KEY_ITUNES_INTEGRATION];
}

- (NSString *)downloadDir
{
	return [defaults objectForKey:PREF_KEY_DOWNLOAD_DIR];
}

- (void)setDownloadDir:(NSString *)downloadDir
{
	[defaults setObject:downloadDir forKey:PREF_KEY_DOWNLOAD_DIR];
}

- (NSString *)iTunesDirectory
{
	return [defaults objectForKey:PREF_KEY_ITUNES_DIRECTORY];
}

- (void)setiTunesDirectory:(NSString *)iTunesDirectory
{
	[defaults setObject:iTunesDirectory forKey:PREF_KEY_ITUNES_DIRECTORY];
}

- (BOOL)createAlbumFiles
{
  return [defaults boolForKey:PREF_KEY_CREATE_ALBUM_FILES];
}

- (void)setCreateAlbumFiles:(BOOL)createAlbumFiles
{
  [defaults setObject:[NSNumber numberWithBool:createAlbumFiles] forKey:PREF_KEY_CREATE_ALBUM_FILES]; 
  
  [theNJB setCreateAlbumFiles:createAlbumFiles];
}

- (BOOL)uploadAlbumArt
{
  return [defaults boolForKey:PREF_KEY_UPLOAD_ALBUM_ART];
}

- (void)setUploadAlbumArt:(BOOL)uploadAlbumArt
{
  [defaults setObject:[NSNumber numberWithBool:uploadAlbumArt] forKey:PREF_KEY_UPLOAD_ALBUM_ART];
  
  [theNJB setUploadAlbumArt:uploadAlbumArt];
}

- (unsigned)albumArtWidth
{
  return [defaults integerForKey:PREF_KEY_ALBUM_ART_WIDTH];
}

- (void)setAlbumArtWidth:(unsigned)albumArtWidth
{
  [defaults setObject:[NSNumber numberWithUnsignedInt:albumArtWidth] forKey:PREF_KEY_ALBUM_ART_WIDTH];
  
  [theNJB setAlbumArtWidth:albumArtWidth];
}

- (unsigned)albumArtHeight
{
  return [defaults integerForKey:PREF_KEY_ALBUM_ART_HEIGHT];
}

- (void)setAlbumArtHeight:(unsigned)albumArtHeight
{
  [defaults setObject:[NSNumber numberWithUnsignedInt:albumArtHeight] forKey:PREF_KEY_ALBUM_ART_HEIGHT];
  
  [theNJB setAlbumArtHeight:albumArtHeight];
}

/**********************/

- (void)registerDefaults
{
	NSMutableDictionary *defaultValues = [NSMutableDictionary dictionary];
	
	// CONNECT_ON_STARTUP
	[defaultValues setObject:DEFAULT_CONNECT_ON_STARTUP forKey:PREF_KEY_CONNECT_ON_STARTUP];
	// STARTUP_TAB
	[defaultValues setObject:DEFAULT_STARTUP_TAB forKey:PREF_KEY_STARTUP_TAB];
	// STARTUP_TAB_LAST_USED
	[defaultValues setObject:DEFAULT_STARTUP_TAB_LAST_USED forKey:PREF_KEY_STARTUP_TAB_LAST_USED];
	// MUSIC_TAB_DIR
	[defaultValues setObject:DEFAULT_MUSIC_TAB_DIR forKey:PREF_KEY_MUSIC_TAB_DIR];
	// MUSIC_TAB_DIR_LAST_USED
	[defaultValues setObject:DEFAULT_MUSIC_TAB_DIR_LAST_USED forKey:PREF_KEY_MUSIC_TAB_DIR_LAST_USED];
	// DATA_TAB_DIR
	[defaultValues setObject:DEFAULT_DATA_TAB_DIR forKey:PREF_KEY_DATA_TAB_DIR];
	// DATA_TAB_DIR_LAST_USED
	[defaultValues setObject:DEFAULT_DATA_TAB_DIR_LAST_USED forKey:PREF_KEY_DATA_TAB_DIR_LAST_USED];
	// FILENAME_FORMAT
	[defaultValues setObject:DEFAULT_FILENAME_FORMAT forKey:PREF_KEY_FILENAME_FORMAT];
	// SHOW_DRAWER
	[defaultValues setObject:DEFAULT_SHOW_DRAWER forKey:PREF_KEY_SHOW_DRAWER];
	// DRAWER_CONTENT_SIZE_HEIGHT
	[defaultValues setObject:DEFAULT_DRAWER_CONTENT_SIZE_HEIGHT forKey:PREF_KEY_DRAWER_CONTENT_SIZE_HEIGHT];
	// DRAWER_CONTENT_SIZE_WIDTH
	[defaultValues setObject:DEFAULT_DRAWER_CONTENT_SIZE_WIDTH forKey:PREF_KEY_DRAWER_CONTENT_SIZE_WIDTH];
	// WRITE_TAG_AFTER_COPY
	[defaultValues setObject:DEFAULT_WRITE_TAG_AFTER_COPY forKey:PREF_KEY_WRITE_TAG_AFTER_COPY];
	// DRAWER_EDGE
	[defaultValues setObject:DEFAULT_DRAWER_EDGE forKey:PREF_KEY_DRAWER_EDGE];
	// DUPLICATES_LENGTH_TOL
	[defaultValues setObject:DEFAULT_DUPLICATES_LENGTH_TOL forKey:PREF_KEY_DUPLICATES_LENGTH_TOL];
	// DUPLICATES_FILESIZE_TOL
	[defaultValues setObject:DEFAULT_DUPLICATES_FILESIZE_TOL forKey:PREF_KEY_DUPLICATES_FILESIZE_TOL];
	// DUPLICATES_TITLE_TOL
	[defaultValues setObject:DEFAULT_DUPLICATES_TITLE_TOL forKey:PREF_KEY_DUPLICATES_TITLE_TOL];
	// DUPLICATES_ARTIST_TOL
	[defaultValues setObject:DEFAULT_DUPLICATES_ARTIST_TOL forKey:PREF_KEY_DUPLICATES_ARTIST_TOL];
	// TURBO
	[defaultValues setObject:DEFAULT_TURBO forKey:PREF_KEY_TURBO];
	// ITUNES_INTEGRATION
	[defaultValues setObject:DEFAULT_PREF_KEY_ITUNES_INTEGRATION forKey:PREF_KEY_ITUNES_INTEGRATION];
	// DOWNLOAD_DIR
	[defaultValues setObject:DEFAULT_PREF_KEY_DOWNLOAD_DIR forKey:PREF_KEY_DOWNLOAD_DIR];
	// ITUNES_DIRECTORY
	[defaultValues setObject:DEFAULT_PREF_KEY_ITUNES_DIRECTORY forKey:PREF_KEY_ITUNES_DIRECTORY];
  // CREATE_ALBUM_FILES
  [defaultValues setObject:DEFAULT_PREF_KEY_CREATE_ALBUM_FILES forKey:PREF_KEY_CREATE_ALBUM_FILES];
  // UPLOAD_ALBUM_ART
  [defaultValues setObject:DEFAULT_PREF_KEY_UPLOAD_ALBUM_ART forKey:PREF_KEY_UPLOAD_ALBUM_ART];
  // ALBUM_ART_WIDTH
  [defaultValues setObject:DEFAULT_PREF_KEY_ALBUM_ART_WIDTH forKey:PREF_KEY_ALBUM_ART_WIDTH];
  // ALBUM_ART_HEIGHT
  [defaultValues setObject:DEFAULT_PREF_KEY_ALBUM_ART_HEIGHT forKey:PREF_KEY_ALBUM_ART_HEIGHT];
	
	[defaults registerDefaults:defaultValues];
}

@end
