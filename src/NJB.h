//
//  NJB.h
//  XNJB
//
//  Created by Richard Low on Wed Jul 21 2004.
//

#import <Foundation/Foundation.h>
#include "libnjb.h"
#import "Track.h"
#import "DataFile.h"
#import "StatusDisplayer.h"
#import "NJBTransactionResult.h"
#import "Playlist.h"
#import "Directory.h"
#include "libmtp.h"
#import "Album.h"

@interface NJB : NSObject {
	IBOutlet StatusDisplayer *statusDisplayer;
	njb_t *njb;
	unsigned debug;
	statusTypes status;
	BOOL connected;
	unsigned long long totalDiskSpace;
	unsigned long long freeDiskSpace;
	NSMutableArray *cachedTrackList;
	Directory *cachedDataFileList;
  NSMutableArray *albums;
	// this is set when we are getting the track list so
	// we know the cache should not be used
	BOOL downloadingTracks;
	BOOL downloadingFiles;
	njb_t njbs[NJB_MAX_DEVICES];
	BOOL turbo;
	BOOL mtpDevice;
	LIBMTP_mtpdevice_t *device;
	uint16_t *supportedFiletypes;
	uint16_t supportedFiletypesLength;
  BOOL createAlbumFiles;
  BOOL uploadAlbumArt;
  unsigned albumArtHeight;
  unsigned albumArtWidth;
  BOOL cancelCurrentTransaction;
	
	NSString *deviceString;
	NSString *firmwareVersionString;
	NSString *deviceIDString;
	NSString *deviceVersionString;
	
	uint8_t batteryLevelMax;
}

- (NJBTransactionResult *)connect;
- (void)disconnect;
- (void)setDebug:(unsigned)newDebug;
- (NSString *)ownerString;
- (statusTypes)status;
- (void)setStatus:(statusTypes)newStatus;
- (BOOL)isConnected;
- (NSMutableArray *)tracks;
- (NSMutableArray *)cachedTrackList;
- (Directory *)cachedDataFileList;
- (Directory *)dataFiles;
- (NSMutableArray *)playlists;
- (NJBTransactionResult *)deleteTrack:(Track *)track;
- (NJBTransactionResult *)uploadTrack:(Track *)track;
- (NJBTransactionResult *)downloadTrack:(Track *)track;
- (NJBTransactionResult *)changeTrackTagTo:(Track *)newTrack from:(Track *)oldTrack;
- (NJBTransactionResult *)downloadFile:(DataFile *)dataFile;
- (NJBTransactionResult *)uploadFile:(DataFile *)dataFile toFolder:(NSString *)path;
- (NJBTransactionResult *)deleteFile:(MyItem *)file fromDir:(Directory *)parentDir;
- (unsigned long long)totalDiskSpace;
- (unsigned long long)freeDiskSpace;
- (NSCalendarDate *)jukeboxTime;
- (BOOL)isProtocol3Device;
- (NJBTransactionResult *)setOwnerString:(NSString *)owner;
- (NJBTransactionResult *)setBitmap:(NSString *)bitmapPath;
- (NJBTransactionResult *)setTime:(NSNumber *)timeIntervalSinceNow;
- (NJBTransactionResult *)updatePlaylist:(Playlist *)playlist;
- (NJBTransactionResult *)deletePlaylist:(Playlist *)playlist;
- (NSString *)productName;
- (NJBTransactionResult *)changeTrackTagTo:(Track *)newTrack from:(Track *)oldTrack;
- (NJBTransactionResult *)createFolder:(Directory *)dir inDir:(NSString *)path;
- (NSString *)deviceString;
- (NSString *)firmwareVersionString;
- (NSString *)deviceIDString;
- (NSString *)deviceVersionString;
- (void)storeDeviceString;
- (void)storeFirmwareVersionString;
- (void)storeDeviceIDString;
- (void)storeDeviceVersionString;
- (int)batteryLevel;
- (NSString *)batteryStatus;
- (void)setTurbo:(BOOL) newTurbo;
- (int)batteryLevelMax;
- (uint32_t)playlistsAssoc;
- (uint32_t)musicAssoc;
- (uint32_t)picturesAssoc;
- (uint32_t)videoAssoc;
- (uint32_t)zencastAssoc;
- (BOOL)isAudioType:(LIBMTP_filetype_t)filetype;
- (BOOL)isMTPDevice;
- (BOOL)isSupportedFileType:(LIBMTP_filetype_t)type;
- (void)downloadAlbumList;
- (Album *)addTrack:(Track *)track ToAlbumWithName:(NSString *)albumName;
- (void)setCreateAlbumFiles:(BOOL)newCreateAlbumFiles;
- (void)setUploadAlbumArt:(BOOL)newUploadAlbumArt;
- (void)setAlbumArtWidth:(unsigned)newAlbumArtWidth;
- (void)setAlbumArtHeight:(unsigned)newAlbumArtHeight;
- (void)cancelCurrentTransaction;
- (NJBTransactionResult *)uploadAlbumArtForTrack:(Track *)track;
- (NJBTransactionResult *)uploadAlbumArt:(NSBitmapImageRep *)imageRep forAlbum:(Album *)album;
- (void)runNotificationLoopWithObject:(id)obj;
- (NJBTransactionResult *)connectWithIterator;

int progress (u_int64_t sent, u_int64_t total, const char* buf, unsigned len, void *data);
int mtp_progress (uint64_t const sent, uint64_t const total, void const * const data);

struct _ProgressParams {
  BOOL *downloadingListing;
  BOOL *cancelCurrentTransaction;
};
typedef struct _ProgressParams ProgressParams;

@end
