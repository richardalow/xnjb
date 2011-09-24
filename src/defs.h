// tabs in main TabView
#define TAB_PLAYLISTS 0
#define TAB_MUSIC 1
#define TAB_DATA 2
#define TAB_SETTINGS 3
#define TAB_DUPLICATES 4
#define TAB_INFO 5
#define TAB_QUEUE 6

// UserDefaults keys and defaults
#define PREF_KEY_CONNECT_ON_STARTUP @"connectOnStartup"
#define DEFAULT_CONNECT_ON_STARTUP [NSNumber numberWithBool:YES]
#define PREF_KEY_STARTUP_TAB @"startupTab"
#define DEFAULT_STARTUP_TAB [NSNumber numberWithInt:TAB_MUSIC]
#define PREF_KEY_STARTUP_TAB_LAST_USED @"startupTabLastUsed"
#define DEFAULT_STARTUP_TAB_LAST_USED [NSNumber numberWithBool:YES]
#define PREF_KEY_MUSIC_TAB_DIR @"musicTabDir"
#define DEFAULT_MUSIC_TAB_DIR [[NSString stringWithString:@"~/Music"] stringByExpandingTildeInPath]
#define PREF_KEY_MUSIC_TAB_DIR_LAST_USED @"musicTabDirLastUsed"
#define DEFAULT_MUSIC_TAB_DIR_LAST_USED [NSNumber numberWithBool:YES]
#define PREF_KEY_DATA_TAB_DIR @"dataTabDir"
#define DEFAULT_DATA_TAB_DIR [[NSString stringWithString:@"~/"] stringByExpandingTildeInPath]
#define PREF_KEY_DATA_TAB_DIR_LAST_USED @"dataTabDirLastUsed"
#define DEFAULT_DATA_TAB_DIR_LAST_USED [NSNumber numberWithBool:YES]
#define PREF_KEY_FILENAME_FORMAT @"filenameFormat"
#define DEFAULT_FILENAME_FORMAT @"%a - %t"
#define PREF_KEY_SHOW_DRAWER @"showDrawer"
#define DEFAULT_SHOW_DRAWER [NSNumber numberWithBool:YES]
#define PREF_KEY_MAIN_WINDOW_FRAME @"mainWindowFrame"
#define PREF_KEY_DRAWER_CONTENT_SIZE_HEIGHT @"drawerContentSizeHeight"
#define DEFAULT_DRAWER_CONTENT_SIZE_HEIGHT [NSNumber numberWithFloat:635.0]
#define PREF_KEY_DRAWER_CONTENT_SIZE_WIDTH @"drawerContentSizeWidth"
#define DEFAULT_DRAWER_CONTENT_SIZE_WIDTH [NSNumber numberWithFloat:270.0]
#define PREF_KEY_MUSIC_TAB_TABLE @"musicTabTable"
#define PREF_KEY_DATA_TAB_TABLE @"dataTabTable"
#define PREF_KEY_QUEUE_TAB_TABLE @"queueTabTable"
#define PREF_KEY_DUPLICATES_TAB_TABLE @"duplicatesTabTable"
#define PREF_KEY_PLAYLISTS_TAB_TABLE @"playlistsTabTable"
#define PREF_KEY_PLAYLISTS_TAB_PLAYLISTS_TABLE @"playlistsTabPlaylistsTable"
#define PREF_KEY_PLAYLISTS_TAB_PLAYLISTS_TRACK_TABLE @"playlistsTabPlaylistsTrackTable"
#define PREF_KEY_WRITE_TAG_AFTER_COPY @"writeTagAfterCopy"
#define DEFAULT_WRITE_TAG_AFTER_COPY [NSNumber numberWithBool:YES]
#define PREF_KEY_DRAWER_EDGE @"drawerEdge"
#define DEFAULT_DRAWER_EDGE [NSNumber numberWithInt:NSMaxXEdge]
#define PREF_KEY_DUPLICATES_LENGTH_TOL @"duplicatesLengthTol"
#define DEFAULT_DUPLICATES_LENGTH_TOL [NSNumber numberWithInt:4]
#define PREF_KEY_DUPLICATES_FILESIZE_TOL @"duplicatesFilesizeTol"
#define DEFAULT_DUPLICATES_FILESIZE_TOL [NSNumber numberWithInt:2]
#define PREF_KEY_DUPLICATES_TITLE_TOL @"duplicatesTitleTol"
#define DEFAULT_DUPLICATES_TITLE_TOL [NSNumber numberWithInt:2]
#define PREF_KEY_DUPLICATES_ARTIST_TOL @"duplicatesArtistTol"
#define DEFAULT_DUPLICATES_ARTIST_TOL [NSNumber numberWithInt:2]
#define PREF_KEY_TURBO @"turbo"
#define DEFAULT_TURBO [NSNumber numberWithBool:YES]
#define PREF_KEY_ITUNES_INTEGRATION @"iTunesIntegration"
#define DEFAULT_PREF_KEY_ITUNES_INTEGRATION [NSNumber numberWithBool:NO]
#define PREF_KEY_DOWNLOAD_DIR @"downloadDir"
#define DEFAULT_PREF_KEY_DOWNLOAD_DIR @"~/Music/XNJB/"
#define PREF_KEY_ITUNES_DIRECTORY @"iTunesDirectory"
#define DEFAULT_PREF_KEY_ITUNES_DIRECTORY @"~/Music/iTunes"
#define PREF_KEY_CREATE_ALBUM_FILES @"createAlbumFiles"
#define DEFAULT_PREF_KEY_CREATE_ALBUM_FILES [NSNumber numberWithBool:YES]
#define PREF_KEY_UPLOAD_ALBUM_ART @"uploadAlbumArt"
#define DEFAULT_PREF_KEY_UPLOAD_ALBUM_ART [NSNumber numberWithBool:YES]
#define PREF_KEY_ALBUM_ART_WIDTH @"albumArtWidth"
#define DEFAULT_PREF_KEY_ALBUM_ART_WIDTH [NSNumber numberWithInt:100]
#define PREF_KEY_ALBUM_ART_HEIGHT @"albumArtHeight"
#define DEFAULT_PREF_KEY_ALBUM_ART_HEIGHT [NSNumber numberWithInt:100]

// Notifications
#define NOTIFICATION_CONNECTED @"XNJBConnected"
#define NOTIFICATION_DISCONNECTED @"XNJBDisconnected"
#define NOTIFICATION_TASK_COMPLETE @"XNJBTaskComplete"
#define NOTIFICATION_TASK_STARTING @"XNJBTaskStarting"
#define NOTIFICATION_APPLICATION_TERMINATING @"XNJBAppTerminating"
// the track list has changed by adding/removing a track. Modifying a track's contents does not trigger this nofitication
#define NOTIFICATION_TRACK_LIST_MODIFIED @"XNJBTrackListModified"