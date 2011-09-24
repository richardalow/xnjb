//
//  Preferences.h
//  XNJB
//
//  Created by Richard Low on Tue Aug 31 2004.
//

#import <Foundation/Foundation.h>
@class MusicTab;
@class DataTab;
@class MainController;
@class DrawerController;
#import "NJB.h"

@interface Preferences : NSObject {
  NSUserDefaults *defaults;
	IBOutlet NJB *theNJB;
}

- (void)registerDefaults;
- (BOOL)connectOnStartup;
- (void)setConnectOnStartup:(BOOL)connectOnStartup;
- (int)startupTab;
- (void)setStartupTab:(int)startupTab;
- (NSString *)musicTabDir;
- (void)setMusicTabDir:(NSString *)musicTabDir;
- (NSString *)dataTabDir;
- (void)setDataTabDir:(NSString *)dataTabDir;
- (void)setMusicTabDirLastUsed:(BOOL)musicTabDirLastUsed;
- (void)setDataTabDirLastUsed:(BOOL)dataTabDirLastUsed;
- (BOOL)musicTabDirLastUsed;
- (BOOL)dataTabDirLastUsed;
- (void)setStartupTabLastUsed:(BOOL)startupTabLastUsed;
- (BOOL)startupTabLastUsed;
- (NSString *)filenameFormat;
- (void)setFilenameFormat:(NSString *)filenameFormat;
- (BOOL)showDrawer;
- (void)setShowDrawer:(BOOL)newShowDrawer;
- (void)setLastUsedMusicTabDir:(NSString *)musicTabDir;
- (void)setLastUsedDataTabDir:(NSString *)dataTabDir;
- (void)setLastUsedTab:(int)tab;
- (void)setDrawerContentSize:(NSSize)size;
- (NSSize)drawerContentSize;
- (NSRectEdge)drawerEdge;
- (void)setDrawerEdge:(NSRectEdge)drawerEdge;
- (BOOL)writeTagAfterCopy;
- (void)setWriteTagAfterCopy:(BOOL)writeTagAfterCopy;
- (int)lengthTol;
- (void)setLengthTol:(int)lengthTol;
- (int)filesizeTol;
- (void)setFilesizeTol:(int)filesizeTol;
- (int)titleTol;
- (void)setTitleTol:(int)titleTol;
- (int)artistTol;
- (void)setArtistTol:(int)artistTol;
- (BOOL)turbo;
- (void)setTurbo:(BOOL)turbo;
- (BOOL)iTunesIntegration;
- (void)setiTunesIntegration:(BOOL)iTunesIntegration;
- (NSString *)downloadDir;
- (void)setDownloadDir:(NSString *)downloadDir;
- (NSString *)iTunesDirectory;
- (void)setiTunesDirectory:(NSString *)iTunesDirectory;
- (BOOL)createAlbumFiles;
- (void)setCreateAlbumFiles:(BOOL)createAlbumFiles;
- (BOOL)uploadAlbumArt;
- (void)setUploadAlbumArt:(BOOL)uploadAlbumArt;
- (unsigned)albumArtWidth;
- (void)setAlbumArtWidth:(unsigned)albumArtWidth;
- (unsigned)albumArtHeight;
- (void)setAlbumArtHeight:(unsigned)albumArtHeight;

@end
