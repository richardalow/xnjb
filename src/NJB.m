//
//  NJB.m
//  XNJB
//
//  Created by Richard Low on Wed Jul 21 2004.
//

/* An obj-c wrapper for libnjb/libmtp
 * most methods return an NJBTransactionResult
 * to indicate success/failure
 */

#import "NJB.h"
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#import "Track.h"
#include "string.h"
#include "base.h"
#include "njb_error.h"
#import "defs.h"
#include <sys/stat.h>
#include <unistd.h>
#import "UnicodeWrapper.h"
#import "Album.h"
#import "QueueTab.h"
#import "NJBQueueConsumer.h"
#include "usb.h"
#include "libusb-glue.h"
#include "usbi.h"
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/usb/IOUSBLib.h>
#include "ptp.h"

// number of tracks in tracklist downloaded in between progress bar updates (MTP only)
#define TRACKLIST_PROGRESS_UPDATE_INTERVAL	10

// declared so can be accessed from C function progress
// set to be the outlet statusDisplayer in awakeFromNib
StatusDisplayer *statusDisplayerGlobal;

// declare the private methods
@interface NJB (PrivateAPI)
- (void)updateDiskSpace;
- (njb_songid_t *)songidStructFromTrack:(Track *)track;
- (NSString *)njbErrorString;
- (NSString *)rateString:(double) rate;
- (NSString *)uniqueFilename:(NSString *)path;
- (LIBMTP_track_t *)libmtpTrack_tFromTrack:(Track *)track;
- (void)libmtpFolder_tToDir:(LIBMTP_folder_t *)folders withDir:(Directory *)baseDir;
// we make this private so that the user never changes the setting directly
// so we only change it before a transfer, so it is not changed during a transfer
// which could mess things up
- (void)enableTurbo;
void deviceAdded(void *refCon, io_iterator_t iterator);
- (void)initDevice;
@end

@implementation NJB

io_iterator_t deviceAddedIter;

// init/dealloc methods

- (id)init
{
	if (self = [super init])
	{
		njb = NULL;
		// 15 is max (this is for libnjb devices)
		[self setDebug:0];
		[self setTurbo:YES];
    [self setCreateAlbumFiles:YES];
		// this defined here for the non MTP devices
		batteryLevelMax = 100;
		
		downloadingTracks = NO;
		downloadingFiles = NO;
    cancelCurrentTransaction = NO;
		
		supportedFiletypes = NULL;
		
		device = NULL;
    
    albums = nil;
    
    LIBMTP_Set_Debug(LIBMTP_DEBUG_NONE);
    
    LIBMTP_Init();
	}
	return self;
}

- (void)dealloc
{
	[statusDisplayerGlobal release];
	[cachedTrackList release];
	[cachedDataFileList release];
	
	[deviceString release];
	[firmwareVersionString release];
	[deviceIDString release];
	[deviceVersionString release];
	
	if (supportedFiletypes != NULL)
		free(supportedFiletypes);
	
	[super dealloc];
}

// accessor methods

- (void)setDebug:(unsigned)newDebug
{
	// see libnjb.h for the Debug flags
	debug = newDebug;
}

- (statusTypes)status
{
	return status;
}

- (void)setStatus:(statusTypes)newStatus
{
	status = newStatus;
	[statusDisplayer setStatus:status];
}

- (BOOL)isConnected
{
	return connected;
}

- (NSString *)deviceString
{
	return deviceString;
}

- (NSString *)firmwareVersionString
{
	return firmwareVersionString;
}

- (NSString *)deviceIDString
{
	return deviceIDString;
}

- (NSString *)deviceVersionString
{
	return deviceVersionString;
}

- (void)setTurbo:(BOOL)newTurbo
{
	turbo = newTurbo;
}

- (int)batteryLevelMax
{
	return batteryLevelMax;
}

- (uint32_t)playlistsAssoc
{
	if (device != NULL)
		return device->default_playlist_folder;
	else
		return 0;
}

- (uint32_t)musicAssoc
{
	if (device != NULL)
		return device->default_music_folder;
	else
		return 0;
}

- (uint32_t)picturesAssoc
{
	if (device != NULL)
		return device->default_picture_folder;
	else
		return 0;
}

- (uint32_t)videoAssoc
{
	if (device != NULL)
		return device->default_video_folder;
	else
		return 0;
}

- (uint32_t)zencastAssoc
{
	if (device != NULL)
		return device->default_zencast_folder;
	else
		return 0;
}

/**********************/

- (void)awakeFromNib
{
	statusDisplayerGlobal = [statusDisplayer retain];
}

- (NSString *)njbErrorString
{
  if (!mtpDevice)
  {
    const char *sp;
    NSMutableString *errorString = [[NSMutableString alloc] init];
    NJB_Error_Reset_Geterror(njb);
    while ((sp = NJB_Error_Geterror(njb)) != NULL)
    {
      [errorString appendString:[NSString stringWithFormat:@"%s, ", sp]];
    }
    njb_error_clear(njb);
    
    NSString *errorString_immutable = [NSString stringWithString:errorString];
    [errorString release];
    return errorString_immutable;
  }
  else
  {
    if (device == NULL)
      return nil;
    
    LIBMTP_error_t *errors = LIBMTP_Get_Errorstack(device);
    
    if (errors == NULL)
      return nil;
    
    LIBMTP_error_t *cur_error = errors;
    NSMutableString *errorString = [[NSMutableString alloc] init];
    
    while(cur_error != NULL)
    {
      // todo: check for NULL error_text
      [errorString appendFormat:@"(%hu) %s;", cur_error->errornumber, cur_error->error_text];
      cur_error = cur_error->next;
    }
    
    LIBMTP_Clear_Errorstack(device);
    
    NSString *errorString_immutable = [NSString stringWithString:errorString];
    [errorString release];
    return errorString_immutable;
  }
}

/*// TEST
#include "libmtp/src/ptp-pack.c"
// !TEST*/


/* connect to the NJB
 */
- (NJBTransactionResult *)connect
{	
	int n;

	connected = NO;
	NSString *errorString = nil;
	
	// try for PDE devices first
	
	if (debug)
		NJB_Set_Debug(debug);
  	
	if (NJB_Discover(njbs, 0, &n) == -1)
	{
		//[self setStatus:STATUS_NO_NJB];
		errorString = NSLocalizedString(@"Could not discover any jukeboxes", nil);
	}
	else
	{
		if (n > 0)
		{
			njb = &njbs[0];
	
			if (NJB_Open(njb) == -1)
			{
				[self setStatus:STATUS_COULD_NOT_OPEN];
				errorString = [self njbErrorString];
			}
			else
			{
				if (NJB_Capture(njb) == -1)
				{
					[self setStatus:STATUS_COULD_NOT_CAPTURE];
					errorString = [self njbErrorString];
					[self disconnect];
				}
				else
				{
					mtpDevice = NO;
					connected = YES;
					[self setStatus:STATUS_CONNECTED];
				}
			}
		}
	}

	if (!connected)
	{
		mtpDevice = YES;
	  
    //usb_set_debug(9);
    
		device = LIBMTP_Get_First_Device();
    
		if (device == NULL)
		{
			connected = NO;
			errorString = NSLocalizedString(@"Could not locate any jukeboxes", nil);
			[self setStatus:STATUS_NO_NJB];
			
			// what about couldn't connect?
			/*
			 errorString = NSLocalizedString(@"Could not talk to jukebox", nil);
			 [self setStatus:STATUS_COULD_NOT_OPEN];
			 */
		}
		else
		{
			connected = YES;
			[self setStatus:STATUS_CONNECTED];
		}
	}
	
	if (!connected)
	{
    return [[[NJBTransactionResult alloc] initWithSuccess:NO resultString:errorString extendedErrorString:[self njbErrorString]] autorelease];
	}
	
	[self initDevice];
  
	// send notification, make sure this is last so all variables are set above
	[statusDisplayer postNotificationName:NOTIFICATION_CONNECTED object:self];
  
  
 /*
  // TEST to read in objectproplist data from a file to debug
  
  NSLog(@"Starting test");

  //NSString *filename = @"/Users/richard/njb/sansa-e280-crash.log";
  NSString *filename = @"/Users/richard/njb/opl-crash-20081027/session-with-usb-bulk-debugging.txt";

  NSFileManager *fileManager = [[NSFileManager alloc] init];
	NSDictionary *fileAttributes = [fileManager fileAttributesAtPath:filename traverseLink:YES];
  unsigned int encodedlen = [[fileAttributes objectForKey:NSFileSize] unsignedIntValue];
  unsigned int len = encodedlen / 2;
  char *data = malloc(len);
  
  int fd = open([filename cString], O_RDONLY);
  
  char cur[2];
  int i = 0;
  
  for (i = 0; i < len; i++)
  {
    read(fd, &cur, 2);
    data[i] = strtol(&cur, NULL, 16);
  }

  MTPProperties *props = NULL;
  
  int nrofprops = ptp_unpack_OPL(device->params, data, &props, len);

  for (i = 0; i < nrofprops; i++)
  {
    NSLog(@"Property 0x%x, data type 0x%x, handle 0x%x", props[i].property, props[i].datatype, props[i].ObjectHandle);
    if (props[i].datatype == PTP_DTC_STR)
      NSLog(@"    prop string value %s", props[i].propval.str);
  }
  
  close(fd);
  free(data);
  // free props
  
  // !TEST
	*/
    
      	
	return [[[NJBTransactionResult alloc] initWithSuccess:YES] autorelease];
}

/* disconnect from the NJB
 */
- (void)disconnect
{
	if ([self isConnected])
	{
		if (!mtpDevice)
		{
			NJB_Release(njb);
			NJB_Close(njb);
		}
		else
		{
			LIBMTP_Release_Device(device);
      if (supportedFiletypes != NULL)
        free(supportedFiletypes);
		}
		connected = NO;
		[self setStatus:STATUS_DISCONNECTED];
		
		// lose the cache
		[cachedTrackList release];
		cachedTrackList = nil;
		[cachedDataFileList release];
		cachedDataFileList = nil;
    [albums release];
    albums = nil;
		
		// send notification
		[statusDisplayer postNotificationName:NOTIFICATION_DISCONNECTED object:self];
	}
}

- (NSString *)productName
{
	if (njb == NULL)
		return nil;
	const char *name = NJB_Get_Device_Name(njb, 1);
	if (name != NULL)
		return [NSString stringWithCString:name];
	else
		return nil;
}

- (NSString *)ownerString
{
  if (![self isConnected])
		return nil;
    
	if (!mtpDevice)
	{
		char *ownerString = NJB_Get_Owner_String(njb);
		if (ownerString == NULL)
			return nil;
		NSString *nsOwnerString = [[NSString alloc] initWithUTF8String:ownerString];
		free(ownerString);
		return [nsOwnerString autorelease];
	}
	else
	{
		char *friendlyName = LIBMTP_Get_Friendlyname(device);
		if (friendlyName != NULL)
		{
			NSString *deviceFriendlyName = [NSString stringWithUTF8String:friendlyName];
			free(friendlyName);
			return deviceFriendlyName;
		}
		else
			return @"";
	}
}

- (NSMutableArray *)tracks
{
	NSLog(@"entering [NJB tracks]");
	
	if (![self isConnected])
		return nil;
	if (cachedTrackList)
	{
		NSLog(@"[NJB tracks] returning cachedTrackList");
		return [self cachedTrackList];
	}
	
	downloadingTracks = YES;
	cachedTrackList = [[NSMutableArray alloc] init];
	if (!mtpDevice)
	{
		NJB_Reset_Get_Track_Tag(njb);	
		njb_songid_t *songtag;
		njb_songid_frame_t *frame;
		while ((songtag = NJB_Get_Track_Tag(njb))) {
			Track *track = [[Track alloc] init];
			
			frame = NJB_Songid_Findframe(songtag, FR_TITLE);
			if (frame != NULL && frame->data.strval != NULL)
				[track setTitle:[NSString stringWithUTF8String:frame->data.strval]];
			
			frame = NJB_Songid_Findframe(songtag, FR_ALBUM);
			if (frame != NULL && frame->data.strval != NULL)
				[track setAlbum:[NSString stringWithUTF8String:frame->data.strval]];
			
			frame = NJB_Songid_Findframe(songtag, FR_ARTIST);
			if (frame != NULL && frame->data.strval != NULL)
				[track setArtist:[NSString stringWithUTF8String:frame->data.strval]];
			
			frame = NJB_Songid_Findframe(songtag, FR_GENRE);
			if (frame != NULL && frame->data.strval != NULL)
				[track setGenre:[NSString stringWithUTF8String:frame->data.strval]];
			
			// this is not used: we don't get extended track info from njb3
			// njb1 gets it but ignored
			frame = NJB_Songid_Findframe(songtag, FR_FNAME);
			if (frame != NULL && frame->data.strval != NULL)
				[track setFilename:[NSString stringWithUTF8String:frame->data.strval]];
			
			frame = NJB_Songid_Findframe(songtag, FR_SIZE);
			if (frame != NULL)
			{
				if (frame->type == NJB_TYPE_UINT16)
					[track setFilesize:frame->data.u_int16_val];
				else
					[track setFilesize:frame->data.u_int32_val];
			}
			frame = NJB_Songid_Findframe(songtag, FR_LENGTH);
			if (frame != NULL)
			{
				if (frame->type == NJB_TYPE_UINT16)
					[track setLength:frame->data.u_int16_val];
				else
					[track setLength:frame->data.u_int32_val];
			}
			frame = NJB_Songid_Findframe(songtag, FR_TRACK);
			if (frame != NULL)
			{
				if (frame->type == NJB_TYPE_UINT16)
					[track setTrackNumber:frame->data.u_int16_val];
				else if (frame->type == NJB_TYPE_UINT32)
					[track setTrackNumber:frame->data.u_int32_val];
				// in case it's a string
				else if (frame->type == NJB_TYPE_STRING && frame->data.strval != NULL)
				{
					NSString *trackNumber = [NSString stringWithCString:frame->data.strval];
					[track setTrackNumber:(unsigned)[trackNumber intValue]];
				}
				else
					NSLog(@"type not expected for FR_TRACK field %d", frame->type);
			}
			frame = NJB_Songid_Findframe(songtag, FR_CODEC);
			if (frame != NULL)
			{
				if (frame->data.strval == NULL)
					[track setFileType:LIBMTP_FILETYPE_UNDEF_AUDIO];
				else
				{
					if (strcmp(frame->data.strval, NJB_CODEC_MP3) == 0)
						[track setFileType:LIBMTP_FILETYPE_MP3];
					else if (strcmp(frame->data.strval, NJB_CODEC_WMA) == 0)
						[track setFileType:LIBMTP_FILETYPE_WMA];
					else if (strcmp(frame->data.strval, NJB_CODEC_WAV) == 0)
						[track setFileType:LIBMTP_FILETYPE_WAV];
					else if (strcmp(frame->data.strval, NJB_CODEC_AA) == 0)
						[track setFileType:LIBMTP_FILETYPE_AUDIBLE];
					else
						[track setFileType:LIBMTP_FILETYPE_UNDEF_AUDIO];
				}
			}
			
			[track setItemID:songtag->trid];

			frame = NJB_Songid_Findframe(songtag, FR_YEAR);
			if (frame != NULL)
			{
				if (frame->type == NJB_TYPE_UINT16)
					[track setYear:frame->data.u_int16_val];
				else if (frame->type == NJB_TYPE_UINT32)
					[track setYear:frame->data.u_int32_val];
				// strings on NJB1
				else if (frame->type == NJB_TYPE_STRING && frame->data.strval != NULL)
				{
					NSString *year = [NSString stringWithCString:frame->data.strval];
					[track setYear:(unsigned)[year intValue]];
				}
				else
					NSLog(@"type not expected for FR_YEAR field %d", frame->type);
			}
			else
				[track setYear:0];
			
			[cachedTrackList addObject:track];
			[track release];
			
			NJB_Songid_Destroy(songtag);
		}
	}
	else
	{		
		NSDate *date = [NSDate date];
				
    ProgressParams progressParams;
    progressParams.downloadingListing = &downloadingTracks;
    BOOL bNo = NO;
    progressParams.cancelCurrentTransaction = &bNo;
    
		LIBMTP_track_t *tracks = LIBMTP_Get_Tracklisting_With_Callback(device, mtp_progress, &progressParams);
		if (tracks == NULL)
		{
      NSString *errorString = [self njbErrorString];
      NSLog(errorString);
      
			downloadingTracks = NO;
			return [self cachedTrackList];
		}
		
		LIBMTP_track_t *track, *tmp;
		track = tracks;
		while (track != NULL)
		{
			Track *myTrack = [[Track alloc] init];
			[myTrack setItemID:track->item_id];
			if (track->title != NULL)
				[myTrack setTitle:[NSString stringWithUTF8String:track->title]];
			if (track->artist != NULL)
				[myTrack setArtist:[NSString stringWithUTF8String:track->artist]];
			if (track->genre != NULL)
				[myTrack setGenre:[NSString stringWithUTF8String:track->genre]];
			if (track->album != NULL)
				[myTrack setAlbum:[NSString stringWithUTF8String:track->album]];
			
			[myTrack setFileType:track->filetype];
			[myTrack setFilesize:track->filesize];
			if (track->filename != NULL)
				[myTrack setFilename:[NSString stringWithUTF8String:track->filename]];
			// duration is in milliseconds
			[myTrack setLength:track->duration/1000];
			[myTrack setTrackNumber:track->tracknumber];
      [myTrack setRating:track->rating];
			
			if (track->date != NULL)
			{
				NSString *nsTimeString = [NSString stringWithUTF8String:track->date];
				// the Creative devices don't send seconds (but they do send tenths of a second!)
				NSCalendarDate *date = [[NSCalendarDate alloc] initWithString:nsTimeString calendarFormat:@"%Y%m%dT%H%M"];
			
				[myTrack setYear:[date yearOfCommonEra]];
        [date release];
			}
			
			[cachedTrackList addObject:myTrack];
			[myTrack release];
			
			// update progress bar
			// TODO: libmtp does not return track count
			/*
			if (i % TRACKLIST_PROGRESS_UPDATE_INTERVAL == 0)
			{
				double progressPercent = (double)i*(double)100.0 / (double)params.handles.n;
				
				[statusDisplayerGlobal updateTaskProgress:progressPercent];
			}*/
			
			tmp = track;
			track = track->next;
			LIBMTP_destroy_track_t(tmp);
		}
		
		[statusDisplayerGlobal updateTaskProgress:100.0];
		
		double time = -[date timeIntervalSinceNow];
		
		NSLog(@"getting tracklist took %f seconds", time);
	}
	
	downloadingTracks = NO;
	
	NSLog(@"leaving [NJB tracks]");
	
	return [self cachedTrackList];
}

/* uploads the track and sets the track id
 */
- (NJBTransactionResult *)uploadTrack:(Track *)track
{
	NSLog(@"uploading track");
	if (![self isConnected])
		return [[[NJBTransactionResult alloc] initWithSuccess:NO resultString:NSLocalizedString(@"Not connected", nil)] autorelease];
	
	if ([[track fullPath] length] == 0)
		return [[[NJBTransactionResult alloc] initWithSuccess:NO resultString:NSLocalizedString(@"Filename empty", nil)] autorelease];
	
	[self setStatus:STATUS_UPLOADING_TRACK];
	
	NSDate *date = [NSDate date];
	
	char* filename = NULL;
	char* lastPathComponent = NULL;
	
	NS_DURING
		filename = (char *) [[track fullPath] fileSystemRepresentation];
		lastPathComponent = (char *) [[[track fullPath] lastPathComponent] fileSystemRepresentation];
	NS_HANDLER
		return [[[NJBTransactionResult alloc] initWithSuccess:NO resultString:NSLocalizedString(@"Could not understand path %@", filename)] autorelease];
	NS_ENDHANDLER
	
	if (!mtpDevice)
	{
		// set turbo on/off
		[self enableTurbo];
		
		unsigned int trackid;
		njb_songid_t *songid = [self songidStructFromTrack:track];
		if (NJB_Send_Track(njb, filename, songid, progress, NULL, &trackid) == -1 ) {
			NSString *error = [self njbErrorString];
			NSLog(error);
			NJB_Songid_Destroy(songid);
			return [[[NJBTransactionResult alloc] initWithSuccess:NO resultString:error] autorelease];
		} else {
			[track setItemID:trackid];
			
			NJB_Songid_Destroy(songid);
		}
	}
	else
	{
		if (![self isSupportedFileType:[track fileType]])
		{
			return [[[NJBTransactionResult alloc] initWithSuccess:NO
																							 resultString:NSLocalizedString(@"Your player does not support this file type: please convert to a different type", nil)] autorelease];
		}
		if (![self isAudioType:[track fileType]])
		{
			return [[[NJBTransactionResult alloc] initWithSuccess:NO
																							 resultString:NSLocalizedString(@"You can't upload non-music files in the music tab - please use the data tab", nil)] autorelease];
		}
		LIBMTP_track_t *trackmeta = [self libmtpTrack_tFromTrack:track];
		if (trackmeta->filename != NULL)
			free(trackmeta->filename);
		trackmeta->filename = strdup(lastPathComponent);
    trackmeta->parent_id = [self musicAssoc];
      
    // clear any errors we don't want so we can see if we get a cancellation error below
    LIBMTP_Clear_Errorstack(device);
      
    ProgressParams progressParams;
    BOOL bNo = NO;
    progressParams.downloadingListing = &bNo;
    cancelCurrentTransaction = NO;
    progressParams.cancelCurrentTransaction = &cancelCurrentTransaction;
    
		if (LIBMTP_Send_Track_From_File(device, filename, trackmeta, mtp_progress, &progressParams) != 0)
		{
			LIBMTP_error_t *errorStack = LIBMTP_Get_Errorstack(device);
			if (errorStack != NULL)
			{
				if (errorStack->errornumber == LIBMTP_ERROR_CANCELLED)
				{
					sleep(1);
					return [[[NJBTransactionResult alloc] initWithSuccess:YES
                                                   resultString:NSLocalizedString(@"Cancelled", nil)] autorelease];	
				}
			}
			NSString *error = @"Error uploading file";
			NSLog(error);
			NSString *errorString = [self njbErrorString];
			NSLog(errorString);
			LIBMTP_destroy_track_t(trackmeta);
			return [[[NJBTransactionResult alloc] initWithSuccess:NO resultString:error extendedErrorString:errorString] autorelease];
		}
		
		[track setItemID:trackmeta->item_id];
    
		if (createAlbumFiles)
		{
			Album *album = [self addTrack:track ToAlbumWithName:[track album]];
			if (uploadAlbumArt)
				[self uploadAlbumArt:[track image] forAlbum:album];
		}

		LIBMTP_destroy_track_t(trackmeta);
	}
	
	double time = -[date timeIntervalSinceNow];
	// rate in Mbps
	double rate = ((double)[track filesize] / time) * 8.0 / (1024.0 * 1024.0);
	
	[self updateDiskSpace];
	
	// update my cache
	// TODO: should we do this?
	[track setImage:nil];
	[track setItcFilePath:@""];
	[cachedTrackList addObject:track];
	// tell others the track list has changed
	[statusDisplayer postNotificationName:NOTIFICATION_TRACK_LIST_MODIFIED object:self];
	
	return [[[NJBTransactionResult alloc] initWithSuccess:YES resultString:[NSString stringWithFormat:NSLocalizedString(@"Speed %@ Mbps", nil), [self rateString:rate]]] autorelease];
}

- (NJBTransactionResult *)deleteTrack:(Track *)track
{
	if (![self isConnected])
		return [[[NJBTransactionResult alloc] initWithSuccess:NO] autorelease];
	
	if (!mtpDevice)
	{
		if (NJB_Delete_Track(njb, [track itemID]) == -1)
		{
			NSString *error = [self njbErrorString];
			NSLog(error);
			return [[[NJBTransactionResult alloc] initWithSuccess:NO resultString:error] autorelease];
		}
	}
	else
	{
		if (LIBMTP_Delete_Object(device, [track itemID]) != 0)
		{
			NSString *error = @"error deleting track";
			NSLog(error);
			NSString *errorString = [self njbErrorString];
			NSLog(errorString);
			return [[[NJBTransactionResult alloc] initWithSuccess:NO resultString:error extendedErrorString:errorString] autorelease];
		}
    
		// TODO: we should delete from playlists too
    
		// delete from album
		if (![[track album] isEqualToString:@""])
		{
			if (albums == nil)
				[self downloadAlbumList];
        
			Album *album;
			NSEnumerator *albumEnumerator = [albums objectEnumerator];
  
			while (album = [albumEnumerator nextObject])
			{
				if ([[album name] caseInsensitiveCompare:[track album]] == NSOrderedSame)
					break;
			}
      
			if (album != nil)
			{
				NSLog(@"deleting track from album");
        
				[album deleteTrackWithID:[track itemID]];
				if ([album trackCount] == 0)
				{
					NSLog(@"deleting empty album with id %d", [album albumID]);
					// delete empty albums
					if (LIBMTP_Delete_Object(device, [album albumID]) != 0)
					{
						NSString *error = @"error deleting album file";
						NSLog(error);
						NSString *errorString = [self njbErrorString];
						NSLog(errorString);
					}
					[albums removeObject:album];
				}
				else
				{
					LIBMTP_album_t *libmtpAlbum = [album toLIBMTPAlbum:track];
          
					if (LIBMTP_Update_Album(device, libmtpAlbum) != 0)
					{
						NSLog(@"Could not update album");
						NSString *errorString = [self njbErrorString];
						NSLog(errorString);
					}

					LIBMTP_destroy_album_t(libmtpAlbum);
				}
			}
		}
	}
	
	[self updateDiskSpace];
	// update my cache
	[cachedTrackList removeObject:track];
	// tell others the track list has changed
	[statusDisplayer postNotificationName:NOTIFICATION_TRACK_LIST_MODIFIED object:self];
		
	return [[[NJBTransactionResult alloc] initWithSuccess:YES] autorelease];
}

- (NJBTransactionResult *)deleteFile:(MyItem *)file fromDir:(Directory *)parentDir
{
	if (![self isConnected])
		return [[[NJBTransactionResult alloc] initWithSuccess:NO] autorelease];
	
	if (!mtpDevice)
	{
		// we can't delete items with 0 ID: these are directories on libnjb devices that
		// haven't been explicitly created
		if ([file itemID] != 0)
		{
			if (NJB_Delete_Datafile(njb, [file itemID]) == -1)
			{
				NSString *error = [self njbErrorString];
				NSLog(error);
				return [[[NJBTransactionResult alloc] initWithSuccess:NO resultString:error] autorelease];
			}
		}
	}
	else
	{
		if (LIBMTP_Delete_Object(device, [file itemID]) != 0)
		{
			NSString *error = @"error deleting file";
			NSLog(error);
            NSString *errorString = [self njbErrorString];
      NSLog(errorString);
      return [[[NJBTransactionResult alloc] initWithSuccess:NO resultString:error extendedErrorString:errorString] autorelease];
		}
	}
	// update cache
	if (parentDir == nil)
		NSLog(@"Cache not being updated since parentDir nil when deleting file %@", file);
	else
		[parentDir removeItem:file];
	
	[self updateDiskSpace];
	return [[[NJBTransactionResult alloc] initWithSuccess:YES] autorelease];
}

- (NJBTransactionResult *)downloadTrack:(Track *)track
{
	// check to see we're not going to overwrite anything
	NSString *fullPath = [track fullPath];
	fullPath = [self uniqueFilename:fullPath];
	[track setFullPath:fullPath];
	
	NSDate *date = [NSDate date];
	
	// todo: is fileSystemRepresentation safe here??
	
	if (!mtpDevice)
	{
		// set turbo on/off
		[self enableTurbo];
	
		if (NJB_Get_Track(njb, [track itemID], [track filesize], [[track fullPath] fileSystemRepresentation], progress, NULL) == -1)
		{
			NSString *error = [self njbErrorString];
			NSLog(error);
			return [[[NJBTransactionResult alloc] initWithSuccess:NO resultString:error] autorelease];
		}
	}
	else
	{
    ProgressParams progressParams;
    BOOL bNo = NO;
    progressParams.downloadingListing = &bNo;
    cancelCurrentTransaction = NO;
    progressParams.cancelCurrentTransaction = &cancelCurrentTransaction;
    
    // clear any errors we don't want so we can see if we get a cancellation error below
    LIBMTP_Clear_Errorstack(device);
    
		if (LIBMTP_Get_File_To_File(device, [track itemID], [[track fullPath] fileSystemRepresentation], mtp_progress, &progressParams) != 0)
		{
      LIBMTP_error_t *errorStack = LIBMTP_Get_Errorstack(device);
      if (errorStack != NULL)
      {
        if (errorStack->errornumber == LIBMTP_ERROR_CANCELLED)
        {
          sleep(1);
          
          return [[[NJBTransactionResult alloc] initWithSuccess:YES
                                                   resultString:NSLocalizedString(@"Cancelled", nil)] autorelease];	
        }
      }
			NSString *error = @"Error downloading file";
			NSLog(error);
      NSString *errorString = [self njbErrorString];
      NSLog(errorString);
      return [[[NJBTransactionResult alloc] initWithSuccess:NO resultString:error extendedErrorString:errorString] autorelease];
		}
	}
	
	double time = -[date timeIntervalSinceNow];
	// rate in Mbps
	double rate = ((double)[track filesize] / time) * 8.0 / (1024.0 * 1024.0);
	
	return [[[NJBTransactionResult alloc] initWithSuccess:YES
																					 resultString:[NSString stringWithFormat:NSLocalizedString(@"Speed %@ Mbps", nil), [self rateString:rate]]] autorelease];
}

- (NSString *)uniqueFilename:(NSString *)path
{
	struct stat sb;
	while (stat([path fileSystemRepresentation], &sb) != -1)
	{
		NSString *extension = [path pathExtension];
		NSString *pathWithoutExtension = [path stringByDeletingPathExtension];
		pathWithoutExtension = [pathWithoutExtension stringByAppendingString:@"_"];
		path = [pathWithoutExtension stringByAppendingPathExtension:extension];
	}
	return path;
}

- (NJBTransactionResult *)changeTrackTagTo:(Track *)newTrack from:(Track *)oldTrack
{
	if (!mtpDevice)
	{
		njb_songid_t *songid = [self songidStructFromTrack:newTrack];
		
		if (NJB_Replace_Track_Tag(njb, [newTrack itemID], songid) == -1)
		{
			NSString *error = [self njbErrorString];
			NSLog(error);
			NJB_Songid_Destroy(songid);
			return [[[NJBTransactionResult alloc] initWithSuccess:NO resultString:error] autorelease];
		}
		else
		{
			NJB_Songid_Destroy(songid);
		}
	}
	else
	{
		LIBMTP_track_t *libmtp_track = [self libmtpTrack_tFromTrack:newTrack];
		if (LIBMTP_Update_Track_Metadata(device, libmtp_track) != 0)
		{
			NSString *error = @"Could not update track info";
			NSLog(error);
			LIBMTP_destroy_track_t(libmtp_track);
      NSString *errorString = [self njbErrorString];
      NSLog(errorString);
      return [[[NJBTransactionResult alloc] initWithSuccess:NO resultString:error extendedErrorString:errorString] autorelease];
		}
		LIBMTP_destroy_track_t(libmtp_track);
    
    if (createAlbumFiles && [newTrack image] && uploadAlbumArt)
    {
      [self uploadAlbumArtForTrack:newTrack];
    }
	}
	
	[self updateDiskSpace];
	
	// update my cache
	[cachedTrackList replaceObject:oldTrack withObject:newTrack];
	
	// tell others the track list has changed
	[statusDisplayer postNotificationName:NOTIFICATION_TRACK_LIST_MODIFIED object:self];
	
	return [[[NJBTransactionResult alloc] initWithSuccess:YES] autorelease];
}

- (Directory *)dataFiles
{
	if (![self isConnected])
		return nil;
	if (cachedDataFileList)
	{
		return [self cachedDataFileList];
	}
	
	downloadingFiles = YES;
	
	cachedDataFileList = [[Directory alloc] init];
	// a better name for here...
	Directory *baseDir = cachedDataFileList;
	
	if (!mtpDevice)
	{
		NJB_Reset_Get_Datafile_Tag(njb);
		njb_datafile_t *filetag;
		while (filetag = NJB_Get_Datafile_Tag(njb))
		{
			NSString *filename = @"";
			if (filetag->filename != NULL)
				filename = [NSString stringWithCString:filetag->filename];
			NSArray *pathArray = nil;
			if (filetag->folder != NULL)
			{
				NSString *folder = [NSString stringWithCString:filetag->folder];
				// this has \\ instead of /
				pathArray = [folder componentsSeparatedByString:@"\\"];
				pathArray = [Directory normalizePathArray:pathArray];
			}

			if ([filename isEqualToString:@"."])
			{
				// this is an empty folder
				NSMutableArray *parentPathArray = [NSMutableArray arrayWithArray:pathArray];
				[parentPathArray removeLastObject];
				
				NSString *name = [pathArray objectAtIndex:[pathArray count] - 1];

				Directory *parentDir = [baseDir itemWithPath:parentPathArray];
				if (![parentDir containsItemWithName:name])
				{
					Directory *newDir = [[Directory alloc] initWithName:name];
					[newDir setItemID:filetag->dfid];
				
					[baseDir addItem:newDir toDir:parentPathArray];
				
					[newDir release];
				}
			}
			else
			{
				// this is a file
				DataFile *dataFile = [[DataFile alloc] init];
				[dataFile setFilename:filename];
				[dataFile setSize:filetag->filesize];
				// [dataFile setTimestampSince1970:filetag->timestamp];
				[dataFile setItemID:filetag->dfid];
				
				[baseDir addItem:dataFile toDir:pathArray];
				
				[dataFile release];
			}
			
			NJB_Datafile_Destroy(filetag);
		}
	}
	else
	{
		// get folders first
		LIBMTP_folder_t *folders = LIBMTP_Get_Folder_List(device);
		[self libmtpFolder_tToDir:folders withDir:baseDir];
		LIBMTP_destroy_folder_t(folders);
		
		NSDictionary *dirDict = [baseDir flatten];
		
		LIBMTP_file_t *files;
		
    ProgressParams progressParams;
    BOOL bNo = NO;
    progressParams.downloadingListing = &downloadingFiles;
    progressParams.cancelCurrentTransaction = &bNo;
    
		files = LIBMTP_Get_Filelisting_With_Callback(device, mtp_progress, &progressParams);
		if (files == NULL)
		{
			downloadingFiles = NO;
      
      NSString *errorString = [self njbErrorString];
      NSLog(errorString);
      
			return nil;
		}
		
		LIBMTP_file_t *file, *tmp;
		file = files;
		while (file != NULL)
		{
			//if (![self isAudioType:file->filetype])
			//{
				DataFile *dataFile = [[DataFile alloc] init];
				if (file->filename != NULL)
					[dataFile setFilename:[NSString stringWithUTF8String:file->filename]];
				[dataFile setSize:file->filesize];
				[dataFile setItemID:file->item_id];
				[dataFile setFileType:file->filetype];
				
				Directory *dir = [dirDict objectForKey:[NSNumber numberWithUnsignedInt:file->parent_id]];
				if (dir == nil)
					dir = baseDir;
				[dir addItem:dataFile toDir:nil];
				
				[dataFile release];
			//}
			
			tmp = file;
			file = file->next;
			LIBMTP_destroy_file_t(tmp);
		}
	}
	
	downloadingFiles = NO;
	return [self cachedDataFileList];
}

- (NJBTransactionResult *)downloadFile:(DataFile *)dataFile
{
	// check to see we're not going to overwrite anything
	NSString *fullPath = [dataFile fullPath];
	fullPath = [self uniqueFilename:fullPath];
	[dataFile setFullPath:fullPath];
		
	NSDate *date = [NSDate date];
		
	if (!mtpDevice)
	{
		// set turbo on/off
		[self enableTurbo];

		if (NJB_Get_File(njb, [dataFile itemID], [dataFile size], [[dataFile fullPath] fileSystemRepresentation], progress, NULL) == -1)
		{
			NSString *error = [self njbErrorString];
			NSLog(error);
			return [[[NJBTransactionResult alloc] initWithSuccess:NO resultString:error] autorelease];
		}
	}
	else
	{
    ProgressParams progressParams;
    BOOL bNo = NO;
    progressParams.downloadingListing = &bNo;
    cancelCurrentTransaction = NO;
    progressParams.cancelCurrentTransaction = &cancelCurrentTransaction;
    
    // clear any errors we don't want so we can see if we get a cancellation error below
    LIBMTP_Clear_Errorstack(device);
    
		if (LIBMTP_Get_File_To_File(device, [dataFile itemID], [[dataFile fullPath] fileSystemRepresentation], mtp_progress, &progressParams) != 0)
		{
      LIBMTP_error_t *errorStack = LIBMTP_Get_Errorstack(device);
      if (errorStack != NULL)
      {
        if (errorStack->errornumber == LIBMTP_ERROR_CANCELLED)
        {
          sleep(1);
          
          return [[[NJBTransactionResult alloc] initWithSuccess:YES
                                                   resultString:NSLocalizedString(@"Cancelled", nil)] autorelease];	
        }
      }
      
			NSString *error = @"Error downloading file";
			NSLog(error);
      NSString *errorString = [self njbErrorString];
      NSLog(errorString);
      return [[[NJBTransactionResult alloc] initWithSuccess:NO resultString:error extendedErrorString:errorString] autorelease];
		}
	}
	
	double time = -[date timeIntervalSinceNow];
	// rate in Mbps
	double rate = ((double)[dataFile size] / time) * 8.0 / (1024.0 * 1024.0);
		
	return [[[NJBTransactionResult alloc] initWithSuccess:YES
																					 resultString:[NSString stringWithFormat:NSLocalizedString(@"Speed %@ Mbps", nil), [self rateString:rate]]] autorelease];
}

- (NJBTransactionResult *)uploadFile:(DataFile *)dataFile toFolder:(NSString *)path
{
	if (![self isConnected])
		return [[[NJBTransactionResult alloc] initWithSuccess:NO] autorelease];
		
	NSDate *date = [NSDate date];
	[self setStatus:STATUS_UPLOADING_FILE];
	
	NSArray *pathArray = [path componentsSeparatedByString:@"\\"];
	pathArray = [Directory normalizePathArray:pathArray];
	
	// we need to make directories if they don't exist
		
	NSMutableArray *destDirPathComponentsSoFar = [[NSMutableArray alloc] initWithCapacity:[pathArray count]];
	int i = 0;
	Directory *parentDir = cachedDataFileList;
	for (i = 0; i < [pathArray count]; i++)
	{
		[destDirPathComponentsSoFar addObject:[pathArray objectAtIndex:i]];
		Directory *dir = [cachedDataFileList itemWithPath:destDirPathComponentsSoFar];
		if (dir == nil)
		{
			Directory *newDir = [[Directory alloc] initWithName:[pathArray objectAtIndex:i]];
			
			// we need to get just parent dir; destDirPathComponentsSoFar includes us too
			NSMutableArray *parentDirComponents = [[NSMutableArray alloc] initWithArray:destDirPathComponentsSoFar];
			// must have at least one item in the array to be here so is safe
			[parentDirComponents removeLastObject];
			NJBTransactionResult *res = [self createFolder:newDir inDir:[Directory stringFromPathArray:parentDirComponents]];
			[parentDirComponents release];
			parentDir = newDir;
			// release here - is retained in createFolder when added to cache
			[newDir release];
			if (![res success])
			{
        NSString *errorString = [self njbErrorString];
        NSLog(errorString);
				return [[[NJBTransactionResult alloc] initWithSuccess:NO
																								 resultString:[NSString stringWithFormat:
																									 @"Could not create containing directory %@", 
																									 [Directory stringFromPathArray:destDirPathComponentsSoFar]] extendedErrorString:errorString] autorelease];
			}
		}
		else
			parentDir = dir;
	}
	
	if (!mtpDevice)
	{
		// set turbo on/off
		[self enableTurbo];
		
		unsigned int fileid;
		if (NJB_Send_File(njb, [[dataFile fullPath] fileSystemRepresentation], [[[dataFile fullPath] lastPathComponent] UTF8String], [path UTF8String], progress, NULL, &fileid) == -1 ) {
			NSString *error = [self njbErrorString];
			NSLog(error);
			return [[[NJBTransactionResult alloc] initWithSuccess:NO resultString:error] autorelease];
		} else {
			[dataFile setItemID:fileid];
		}
	}
	else
	{
		uint16_t objectFormat = [dataFile fileType];
		uint32_t parenthandle = [parentDir itemID];
		
		LIBMTP_file_t *genfile = LIBMTP_new_file_t();
		genfile->filesize = [dataFile size];
		genfile->filetype = objectFormat;
		genfile->filename = strdup([[[dataFile fullPath] lastPathComponent] fileSystemRepresentation]);
    genfile->parent_id = parenthandle;
		
    // clear any errors we don't want so we can see if we get a cancellation error below
    LIBMTP_Clear_Errorstack(device);
    
    ProgressParams progressParams;
    BOOL bNo = NO;
    progressParams.downloadingListing = &bNo;
    cancelCurrentTransaction = NO;
    progressParams.cancelCurrentTransaction = &cancelCurrentTransaction;
    
		if (LIBMTP_Send_File_From_File(device, [[dataFile fullPath] fileSystemRepresentation], genfile, mtp_progress, &progressParams) != 0)
		{
      LIBMTP_error_t *errorStack = LIBMTP_Get_Errorstack(device);
      if (errorStack != NULL)
      {
        if (errorStack->errornumber == LIBMTP_ERROR_CANCELLED)
        {
          sleep(1);
          
          return [[[NJBTransactionResult alloc] initWithSuccess:YES
                                                   resultString:NSLocalizedString(@"Cancelled", nil)] autorelease];	
        }
      }
      
			/* we might get an (MTP error) object too large here */
			NSString *error = @"Error uploading file";
			NSLog(error);
			LIBMTP_destroy_file_t(genfile);
      NSString *errorString = [self njbErrorString];
      NSLog(errorString);
      return [[[NJBTransactionResult alloc] initWithSuccess:NO resultString:error extendedErrorString:errorString] autorelease];
		}
		
		[dataFile setItemID:genfile->item_id];
		LIBMTP_destroy_file_t(genfile);
	}
	// update the cache
	[parentDir addItem:dataFile];
		
	[self updateDiskSpace];
		
	double time = -[date timeIntervalSinceNow];
	// rate in Mbps
	double rate = ((double)[dataFile size] / time) * 8.0 / (1024.0 * 1024.0);
	return [[[NJBTransactionResult alloc] initWithSuccess:YES
																					 resultString:[NSString stringWithFormat:NSLocalizedString(@"Speed %@ Mbps", nil), [self rateString:rate]]] autorelease];	
}

- (unsigned long long)totalDiskSpace
{
	return totalDiskSpace;
}

- (unsigned long long)freeDiskSpace
{
	return freeDiskSpace;
}

- (void)updateDiskSpace
{
	if (!mtpDevice)
	{
		NJB_Get_Disk_Usage(njb, &totalDiskSpace, &freeDiskSpace);
		[statusDisplayer updateDiskSpace:totalDiskSpace withFreeSpace:freeDiskSpace];
	}
	else
	{
		if (device == NULL || LIBMTP_Get_Storage(device, 0) == -1)
			return;
		
		if (device->storage == NULL)
			return;
		
		totalDiskSpace = device->storage->MaxCapacity;
		freeDiskSpace = device->storage->FreeSpaceInBytes;
		
		[statusDisplayer updateDiskSpace:totalDiskSpace withFreeSpace:freeDiskSpace];
	}
}

- (NSCalendarDate *)jukeboxTime
{
	if (!mtpDevice)
	{
		njb_time_t *time = NJB_Get_Time(njb);
		if (time == NULL)
			return nil;
	
		// assume njb time is our timezone
		NSCalendarDate *date = [NSCalendarDate dateWithYear:time->year month:time->month day:time->day hour:time->hours minute:time->minutes second:time->seconds
																							 timeZone:[NSTimeZone localTimeZone]];
		return date;
	}
	else
	{
		/*char *secureTime = NULL;
		if (LIBMTP_Get_Secure_Time(device, &secureTime) != 0)
			return nil;
		
		if (secureTime == NULL)
			return nil;
		
		NSLog(@"Secure time: %s", secureTime);
		
		free(secureTime);*/
		// TODO
		/*
		if (!ptp_property_issupported(&params, PTP_DPC_DateTime))
			return nil;
		
		char *timeString = NULL;
		if (ptp_getdevicepropvalue(&params, PTP_DPC_DateTime, (void **)&timeString, PTP_DTC_STR) != PTP_RC_OK)
			return nil;
		
		if (timeString != NULL)
		{
			NSString *nsTimeString = [NSString stringWithCString:timeString];
			free(timeString);
			NSCalendarDate *date = [[NSCalendarDate alloc] initWithString:nsTimeString calendarFormat:@"%Y%m%dT%H%M%S%z"];
		
			return [date autorelease];
		}
		else*/
			return nil;
	}
}

- (BOOL)isProtocol3Device
{
	if (!mtpDevice)
		return (PDE_PROTOCOL_DEVICE(njb));
	else
		return NO;
}

- (NJBTransactionResult *)setOwnerString:(NSString *)owner
{
	if (!mtpDevice)
	{
		if (NJB_Set_Owner_String (njb, [owner UTF8String]) == -1)
		{
			NSString *error = [self njbErrorString];
			NSLog(error);
			return [[[NJBTransactionResult alloc] initWithSuccess:NO resultString:error] autorelease];
		}
	}
	else
	{
		if (LIBMTP_Set_Friendlyname(device, [owner UTF8String]) != 0)
		{
			NSString *error = [NSString stringWithFormat:@"Could not set device friendly name"];
			NSLog(error);
      NSString *errorString = [self njbErrorString];
      NSLog(errorString);
      return [[[NJBTransactionResult alloc] initWithSuccess:NO resultString:error extendedErrorString:errorString] autorelease];
		}
	}
	return [[[NJBTransactionResult alloc] initWithSuccess:YES] autorelease];
}

- (NJBTransactionResult *)setBitmap:(NSString *)bitmapPath
{
	NSLog(@"setBitmap not implemented!");
	return [[[NJBTransactionResult alloc] initWithSuccess:NO] autorelease];
	/*if (mtpDevice)
		return [[[NJBTransactionResult alloc] initWithSuccess:NO] autorelease];
	
	if (NJB_Set_Bitmap(njb, [bitmapPath UTF8String]) == -1)
	{
		NSString *error = [self njbErrorString];
		NSLog(error);
		return [[[NJBTransactionResult alloc] initWithSuccess:NO resultString:error] autorelease];
	}
	return [[[NJBTransactionResult alloc] initWithSuccess:YES] autorelease];*/
}

- (NJBTransactionResult *)setTime:(NSNumber *)timeIntervalSinceNow
{
	if (mtpDevice)
		return [[[NJBTransactionResult alloc] initWithSuccess:NO] autorelease];
	
	njb_time_t time;
	
	NSTimeInterval jukeboxTimeInterval = [timeIntervalSinceNow doubleValue];
	NSCalendarDate *date = [NSCalendarDate dateWithTimeIntervalSinceNow:jukeboxTimeInterval];
	
	time.year = [date yearOfCommonEra];
	time.month = [date monthOfYear];
	time.day = [date dayOfMonth];
	time.weekday = [date dayOfWeek];
	time.hours = [date hourOfDay];
	time.minutes = [date minuteOfHour];
	time.seconds = [date secondOfMinute];
	
	if (NJB_Set_Time(njb, &time) == -1)
	{
		NSString *error = [self njbErrorString];
		NSLog(error);
		return [[[NJBTransactionResult alloc] initWithSuccess:NO resultString:error] autorelease];
	}
	return [[[NJBTransactionResult alloc] initWithSuccess:YES] autorelease];
}

- (NSMutableArray *)playlists
{
	if (![self isConnected])
		return nil;
	
	NSMutableArray *playlists = [[NSMutableArray alloc] init];
	
	if (!mtpDevice)
	{
		NJB_Reset_Get_Playlist(njb);
		njb_playlist_t *playlist;
		while ((playlist = NJB_Get_Playlist(njb)))
		{
			Playlist *newPlaylist = [[Playlist alloc] init];
					
			const char *str = playlist->name;
			if (str != NULL)
				[newPlaylist setName:[NSString stringWithUTF8String:str]];
			[newPlaylist setPlaylistID:playlist->plid];
			
			njb_playlist_track_t *track;
			NJB_Playlist_Reset_Gettrack(playlist);
			while ((track = NJB_Playlist_Gettrack(playlist)))
			{
				Track *newTrack = [[Track alloc] init];
				[newTrack setItemID:track->trackid];
				[newPlaylist addTrack:newTrack];
				[newTrack release];
			}
			
			NJB_Playlist_Destroy(playlist);
			
			[newPlaylist setState:NJB_PL_UNCHANGED];
			
			[playlists addObject:newPlaylist];
			[newPlaylist release];
		}
	}
	else
	{
		LIBMTP_playlist_t *libmtpPlaylists = LIBMTP_Get_Playlist_List(device);
		if (libmtpPlaylists == NULL)
		{
			// no playlists
			return [playlists autorelease];
		}
		LIBMTP_playlist_t *pl, *tmp;
    pl = libmtpPlaylists;
    while (pl != NULL) {
			Playlist *newPlaylist = [[Playlist alloc] init];
			
			[newPlaylist setPlaylistID:pl->playlist_id];
			if (pl->name != NULL)
				[newPlaylist setName:[NSString stringWithUTF8String:pl->name]];
			int i = 0;
			for (i = 0; i < pl->no_tracks; i++)
			{
				Track *newTrack = [[Track alloc] init];
				[newTrack setItemID:pl->tracks[i]];
				[newPlaylist addTrack:newTrack];
				[newTrack release];
			}
			
			[newPlaylist setState:NJB_PL_UNCHANGED];
			
			[playlists addObject:newPlaylist];
			[newPlaylist release];

      tmp = pl;
      pl = pl->next;
      LIBMTP_destroy_playlist_t(tmp);
    }
	}
	return [playlists autorelease];
}

/* updates the playlist on the NJB
 * may add a new playlist even if playlist state is not new
 * so the playlist ID may change.
 */
- (NJBTransactionResult *)updatePlaylist:(Playlist *)playlist
{
	[playlist lock];
	// don't do anything if it's unchanged
	if ([playlist state] == NJB_PL_UNCHANGED)
	{
		[playlist unlock];
		return [[[NJBTransactionResult alloc] initWithSuccess:YES] autorelease];
	}
	
	// the playlist is locked here
	if (!mtpDevice)
	{
		// create new njb_playlist_t object
		njb_playlist_t *pl = NJB_Playlist_New();
		NJB_Playlist_Set_Name(pl, [[playlist name] UTF8String]);
		pl->plid = [playlist playlistID];
		NSEnumerator *enumerator = [[playlist tracks] objectEnumerator];
		Track *currentTrack;
		while (currentTrack = [enumerator nextObject])
		{
			njb_playlist_track_t *newTrack = NJB_Playlist_Track_New([currentTrack itemID]);
			NJB_Playlist_Addtrack(pl, newTrack, NJB_PL_END);
		}
		pl->_state = [playlist state];
		// set this now as as far as the caller is concerned the NJB is updated
		[playlist setState:NJB_PL_UNCHANGED];
		[playlist unlock];
		if (NJB_Update_Playlist(njb, pl) == -1)
		{
			NJB_Playlist_Destroy(pl);
			NSString *error = [self njbErrorString];
			NSLog(error);
			return [[[NJBTransactionResult alloc] initWithSuccess:NO resultString:error] autorelease];
		}
		[playlist lock];
		// update the ID as this is changed when tracks added/removed as a new playlist is created
		[playlist setPlaylistID:pl->plid];
		[playlist unlock];
		NJB_Playlist_Destroy(pl);
	}
	else
	{
		NSArray *tracks = [playlist tracks];
		uint32_t size = [tracks count];
		uint32_t *trackArray = (uint32_t*)malloc(sizeof(uint32_t)*size);
		uint32_t i = 0;
		for (i = 0; i < size; i++)
			trackArray[i] = [[tracks objectAtIndex:i] itemID];
		
		LIBMTP_playlist_t *libmtpPlaylist = LIBMTP_new_playlist_t();
		libmtpPlaylist->name = strdup([[playlist name] UTF8String]);
		libmtpPlaylist->playlist_id = [playlist playlistID];
		libmtpPlaylist->no_tracks = size;
		libmtpPlaylist->tracks = trackArray;
    libmtpPlaylist->parent_id = device->default_playlist_folder;
		
		int playlistState = [playlist state];
		[playlist setState:NJB_PL_UNCHANGED];
		[playlist unlock];
		
		if (playlistState == NJB_PL_NEW)
		{
			if (LIBMTP_Create_New_Playlist(device, libmtpPlaylist) != 0)
			{
				LIBMTP_destroy_playlist_t(libmtpPlaylist);
				NSString *errorString = [self njbErrorString];
				NSLog(errorString);
				return [[[NJBTransactionResult alloc] initWithSuccess:NO resultString:
					[NSString stringWithFormat:@"Could not create new playlist"] extendedErrorString:errorString] autorelease];
			}
			[playlist lock];
			[playlist setPlaylistID:libmtpPlaylist->playlist_id];
			[playlist unlock];
		}
		else
		{
			if (LIBMTP_Update_Playlist(device, libmtpPlaylist) != 0)
			{
				LIBMTP_destroy_playlist_t(libmtpPlaylist);
				NSString *errorString = [self njbErrorString];
				NSLog(errorString);        
				return [[[NJBTransactionResult alloc] initWithSuccess:NO resultString:
					[NSString stringWithFormat:@"Could not update playlist"] extendedErrorString:errorString] autorelease];
			}
			[playlist lock];
			// it doesn't change the playlist now but put it in to be
			// on the safe side
			[playlist setPlaylistID:libmtpPlaylist->playlist_id];
			[playlist unlock];
		}
		LIBMTP_destroy_playlist_t(libmtpPlaylist);
	}
	
	return [[[NJBTransactionResult alloc] initWithSuccess:YES] autorelease];
}

- (NJBTransactionResult *)deletePlaylist:(Playlist *)playlist
{	
	[playlist lock];
	unsigned plid = [playlist playlistID];
	[playlist unlock];
	
	if (!mtpDevice)
	{
		if (NJB_Delete_Playlist(njb, plid) == -1)
		{
			NSString *error = [self njbErrorString];
			NSLog(error);
			return [[[NJBTransactionResult alloc] initWithSuccess:NO resultString:error] autorelease];
		}
	}
	else
	{
		if (LIBMTP_Delete_Object(device, [playlist playlistID]) != 0)
		{
			NSString *error = @"error deleting playlist";
			NSLog(error);
      NSString *errorString = [self njbErrorString];
      NSLog(errorString);
      return [[[NJBTransactionResult alloc] initWithSuccess:NO resultString:error extendedErrorString:errorString] autorelease];
		}
	}
	
	return [[[NJBTransactionResult alloc] initWithSuccess:YES] autorelease];
}

/* called by the file/track transfer functions of libnjb
 * to tell us the progress
 */
int progress(u_int64_t sent, u_int64_t total, const char* buf, unsigned len, void *data)
{
	double percent = (double)sent*100.0/(double)total;
	[statusDisplayerGlobal updateTaskProgress:percent];
	
	return 0;
}

int mtp_progress(uint64_t const sent, uint64_t const total, void const * const data)
{
  ProgressParams *progressParams = (ProgressParams *)data;
  // if we're downloading a the file or track listing don't update on every call
  if (data != NULL && *(progressParams->downloadingListing))
  {
   if (sent % TRACKLIST_PROGRESS_UPDATE_INTERVAL != 0)
    return 0;
  }
	double percent = (double)sent*100.0/(double)total;
	[statusDisplayerGlobal updateTaskProgress:percent];

  if (*(progressParams->cancelCurrentTransaction))
  {
    return 1;
    *(progressParams->cancelCurrentTransaction) = NO;
  }
  else
    return 0;
}

- (njb_songid_t *)songidStructFromTrack:(Track *)track
{
	njb_songid_t *songid;
	njb_songid_frame_t *frame;
	
	songid = NJB_Songid_New();
	if ([track njbCodec])
	{
		frame = NJB_Songid_Frame_New_Codec([[track njbCodec] UTF8String]);
		NJB_Songid_Addframe(songid, frame);
	}
	// only add file size if non zero
	if ([track filesize] != 0)
	{
		frame = NJB_Songid_Frame_New_Filesize([track filesize]);
		NJB_Songid_Addframe(songid, frame);
	}
	if ([track title])
	{
		frame = NJB_Songid_Frame_New_Title([[track title] UTF8String]);
		NJB_Songid_Addframe(songid, frame);
	}
	if ([track album])
	{
		frame = NJB_Songid_Frame_New_Album([[track album] UTF8String]);
		NJB_Songid_Addframe(songid, frame);
	}
	if ([track artist])
	{
		frame = NJB_Songid_Frame_New_Artist([[track artist] UTF8String]);
		NJB_Songid_Addframe(songid, frame);
	}
	if ([track genre])
	{
		frame = NJB_Songid_Frame_New_Genre([[track genre] UTF8String]);
		NJB_Songid_Addframe(songid, frame);
	}
	frame = NJB_Songid_Frame_New_Year([track year]);
	NJB_Songid_Addframe(songid, frame);
	frame = NJB_Songid_Frame_New_Tracknum([track trackNumber]);
	NJB_Songid_Addframe(songid, frame);
	frame = NJB_Songid_Frame_New_Length([track length]);
	NJB_Songid_Addframe(songid, frame);
	if ([track fullPath] && [[track fullPath] length] > 0)
	{
		frame = NJB_Songid_Frame_New_Filename([[track fullPath] fileSystemRepresentation]);
		NJB_Songid_Addframe(songid, frame);
	}
	
	return songid;
}

- (NSMutableArray *)cachedTrackList
{
	if (cachedTrackList && !downloadingTracks)
	{
		NSMutableArray *tracks = [[NSMutableArray alloc] initWithArray:cachedTrackList];
		return [tracks autorelease];
	}
	else
		return nil;
}

- (Directory *)cachedDataFileList
{
	if (cachedDataFileList && !downloadingFiles)
	{
		return [[cachedDataFileList copy] autorelease];
	}
	else
		return nil;
}

/* returns a string rounded to 1dp for
 * the rate given
 */
- (NSString *)rateString:(double) rate
{
	int rate10 = rate * 10;
	if (rate - (double)rate10/10.0 >= 0.05)
		rate10++;
	// the two digits
	int rateA = rate10/10;
	int rateB = rate10-rateA*10;
	return [NSString stringWithFormat:@"%d.%d", rateA, rateB];
}

- (NJBTransactionResult *)createFolder:(Directory *)dir inDir:(NSString *)path
{
	NSArray *pathArray = [path componentsSeparatedByString:@"\\"];
	pathArray = [Directory normalizePathArray:pathArray];
	Directory *parentDir = [cachedDataFileList itemWithPath:pathArray];
	
	if (!mtpDevice)
	{
		unsigned int folderID;
		if (NJB_Create_Folder(njb, [[NSString stringWithFormat:@"%@%@", path, [dir name]] UTF8String], &folderID) == -1)
		{
			NSString *error = [self njbErrorString];
			NSLog(error);
			return [[[NJBTransactionResult alloc] initWithSuccess:NO resultString:error] autorelease];
		}
		[dir setItemID:folderID];
	}
	else
	{
		unsigned newDirID = LIBMTP_Create_Folder(device, [[dir name] UTF8String], [parentDir itemID], 0);
		if (newDirID == 0)
		{
      NSString *errorString = [self njbErrorString];
      NSLog(errorString);
      
			return [[[NJBTransactionResult alloc] initWithSuccess:NO 
																							 resultString:[NSString stringWithFormat:@"Could not create folder with parent ID 0x%04.", [parentDir itemID]]
                                        extendedErrorString:errorString] autorelease];
		}
		[dir setItemID:newDirID];
	}
	// update cache
	[parentDir addItem:dir];
	
	return [[[NJBTransactionResult alloc] initWithSuccess:YES] autorelease];
}

- (void)storeDeviceString
{
	if (!mtpDevice)
	{
		const char *name = NJB_Get_Device_Name(njb, 0);
		if (name != NULL)
			deviceString = [[NSString alloc] initWithCString:name];
		else
			deviceString = nil;
	}
	else
		deviceString = @"";
}

- (void)storeFirmwareVersionString
{
	if (!mtpDevice)
	{
		u_int8_t major, minor, release;
			
		int ret = NJB_Get_Firmware_Revision(njb, &major, &minor, &release);
		if (ret == 0)
			firmwareVersionString = [[NSString alloc] initWithString:[NSString stringWithFormat:@"%u.%u.%u", major, minor, release]];
		else
			deviceVersionString = nil;
	}
	else
		firmwareVersionString = @"";
}

- (void)storeDeviceIDString
{	
	if (!mtpDevice)
	{
		NSMutableString *idString = [[NSMutableString alloc] initWithCapacity:32];
		int j;
		
		u_int8_t sdmiid[16];
		int result;
		result = NJB_Get_SDMI_ID(njb, &(sdmiid[0]));
		if (result != 0)
			idString = @"";
		
		for (j = 0; j < 16; j++)
		{
			if (sdmiid[j] > 15)
				[idString appendString:[NSString stringWithFormat:@"%X", sdmiid[j]]];
			else
				[idString appendString:[NSString stringWithFormat:@"0%X", sdmiid[j]]];
		}
		
		deviceIDString = [[NSString alloc] initWithString:[NSString stringWithString:idString]];
		[idString release];
	}
	else
		deviceIDString = @"";
}

- (void)storeDeviceVersionString
{
	if (!mtpDevice)
	{
		u_int8_t major, minor, release;
			
		int ret = NJB_Get_Hardware_Revision(njb, &major, &minor, &release);
		if (ret == 0)
			deviceVersionString = [[NSString alloc] initWithString:[NSString stringWithFormat:@"%u.%u.%u", major, minor, release]];
		else
			deviceVersionString = nil;
	}
	else
		deviceVersionString = @"";
}

- (int)batteryLevel
{
	if (connected == NO)
		return 0;
	if (!mtpDevice)
	{
		return NJB_Get_Battery_Level(njb);
	}
	else
	{				
		uint8_t value = 0;
		if (LIBMTP_Get_Batterylevel(device, &batteryLevelMax, &value) != 0)
		{
			NSLog(@"Could not get battery level property value");
			return 0;
		}
    				
		return value;
	}
}

- (NSString *)batteryStatus
{
	if (!mtpDevice)
	{
		BOOL charging = (NJB_Get_Battery_Charging(njb) == 1);
		BOOL powerConnected = (NJB_Get_Auxpower(njb) == 1);
		
		if (charging)
			return NSLocalizedString(@"Charging", nil);
		if (powerConnected)
			return NSLocalizedString(@"Battery charged or not present", nil);
		return NSLocalizedString(@"Running off battery", nil);
	}
	else
	{
		// they always charge...
		// TODO: they don't - they can be fully charged. Can we see that?
		return NSLocalizedString(@"Charging", nil);
	}
}

- (void)enableTurbo
{
	//NSLog(@"enableTurbo : %d", turbo);

	if (turbo)
		NJB_Set_Turbo_Mode(njb, NJB_TURBO_ON);
	else
		NJB_Set_Turbo_Mode(njb, NJB_TURBO_OFF);
}

// TODO: libmtp has this in LIBMTP_Get_Tracklisting - they should be the same function
- (BOOL)isAudioType:(LIBMTP_filetype_t) filetype
{
	if (filetype == LIBMTP_FILETYPE_WAV ||
			filetype == LIBMTP_FILETYPE_MP3 ||
			filetype == LIBMTP_FILETYPE_WMA ||
			filetype == LIBMTP_FILETYPE_OGG ||
			filetype == LIBMTP_FILETYPE_AUDIBLE ||
			filetype == LIBMTP_FILETYPE_MP4 ||
			filetype == LIBMTP_FILETYPE_UNDEF_AUDIO ||
      filetype == LIBMTP_FILETYPE_AAC ||
      filetype == LIBMTP_FILETYPE_MEDIACARD ||
      filetype == LIBMTP_FILETYPE_FLAC ||
      filetype == LIBMTP_FILETYPE_MP2 ||
      filetype == LIBMTP_FILETYPE_M4A
			)
		return YES;
	else
		return NO;
}

// return value must be freed (LIBMTP_destroy_track_t) by caller
- (LIBMTP_track_t *)libmtpTrack_tFromTrack:(Track *)track
{
	LIBMTP_track_t *libmtp_track = LIBMTP_new_track_t();
	
	libmtp_track->filetype = [track fileType];
		
	// we need to have title, album and artist set else some players get confused
	if ([[track title] length] == 0)
		libmtp_track->title = strdup("Unknown title");
	else
		libmtp_track->title = strdup([[track title] UTF8String]);
	
	if ([[track album] length] == 0)
		libmtp_track->album = strdup("Unknown album");
	else
		libmtp_track->album = strdup([[track album] UTF8String]);
	
	if ([[track artist] length] == 0)
		libmtp_track->artist = strdup("Unknown artist");
	else
		libmtp_track->artist = strdup([[track artist] UTF8String]);
	
	if ([[track genre] length] > 0)
		libmtp_track->genre = strdup([[track genre] UTF8String]);
	else
		libmtp_track->genre = strdup("");
	
	NSString *nsTimeString = [NSString stringWithFormat:@"%.04d0101T000000.0", [track year]];
	libmtp_track->date = strdup([nsTimeString UTF8String]);
	libmtp_track->tracknumber = [track trackNumber];
	libmtp_track->duration = [track length] * 1000;
	libmtp_track->filesize = [track filesize];
	if ([[track filename] length] > 0)
		libmtp_track->filename = strdup([[track filename] UTF8String]);
	else
		libmtp_track->filename = NULL;
  libmtp_track->rating = [track rating];
	libmtp_track->item_id = [track itemID];
		
	return libmtp_track;
}

- (void)libmtpFolder_tToDir:(LIBMTP_folder_t *)folders withDir:(Directory *)baseDir
{
	if (folders == NULL)
		return;
	
	NSString *name = @"(unnamed directory)";
	if (folders->name != NULL)
		name = [NSString stringWithUTF8String:folders->name];

	Directory *thisDir = [baseDir addNewDirectory:[NSArray arrayWithObjects:name, nil]];
	[thisDir setItemID:folders->folder_id];
	
	[self libmtpFolder_tToDir:folders->sibling withDir:baseDir];
	
	[self libmtpFolder_tToDir:folders->child withDir:thisDir];
}

- (BOOL)isMTPDevice
{
	return mtpDevice;
}

- (BOOL)isSupportedFileType:(LIBMTP_filetype_t)type
{
	if (!mtpDevice)
		return YES;
  
  if (supportedFiletypesLength == 0)
  {
    NSLog(@"Player claims it supports no types; we will allow all");
    return YES;
  }
	
  NSLog(@"checking to see if we support type %d", type);
  
	int i = 0;
	for (i = 0; i < supportedFiletypesLength; i++)
	{
    NSLog(@"supported type: %d", supportedFiletypes[i]);
		if (supportedFiletypes[i] == type)
			return YES;
	}
	return NO;
}

- (void)downloadAlbumList
{
	if (![self isConnected] || !mtpDevice)
		return;
	
  if (albums != nil)
    [albums release];
  
	albums = [[NSMutableArray alloc] init];
	
  LIBMTP_album_t *libmtpAlbums = LIBMTP_Get_Album_List(device);
  if (libmtpAlbums == NULL)
  {
    // no albums or we failed
    return;
  }
  LIBMTP_album_t *alb, *tmp;
  alb = libmtpAlbums;
  while (alb != NULL) {
		Album *newAlbum = [[Album alloc] init];
			
    [newAlbum setAlbumID:alb->album_id];
    if (alb->name != NULL)
      [newAlbum setName:[NSString stringWithUTF8String:alb->name]];
    int i = 0;
    for (i = 0; i < alb->no_tracks; i++)
    {
      Track *newTrack = [[Track alloc] init];
      [newTrack setItemID:alb->tracks[i]];
      [newAlbum addTrack:newTrack];
      [newTrack release];
    }
    [newAlbum setState:NJB_PL_UNCHANGED];
			
    [albums addObject:newAlbum];
    [newAlbum release];
      
    tmp = alb;
    alb = alb->next;
    LIBMTP_destroy_album_t(tmp);
	}
}

- (Album *)addTrack:(Track *)track ToAlbumWithName:(NSString *)albumName
{
  NSLog (@"adding track with id %d to album name %@", [track itemID], albumName);
  // don't add to a blank album
  if ([albumName isEqualToString:@""])
    return nil;
  
  if (albums == nil)
    [self downloadAlbumList];
  
  Album *album;
  NSEnumerator *albumEnumerator = [albums objectEnumerator];
  
  while (album = [albumEnumerator nextObject])
  {
    if ([[album name] caseInsensitiveCompare:albumName] == NSOrderedSame)
      break;
  }
  
  if (album == nil)
  {
    // make new album
    album = [[Album alloc] init];
    [album setName:albumName];
    [album addTrack:track];
    
    [albums addObject:album];
    [album release];
  }
  else
  {
    // add track to existing album
    [album addTrack:track];
    
    if ([album albumID] == 0)
    {
      NSLog(@"Found album, but has 0 ID, giving up.");
      return nil;
    }
  }
  
  LIBMTP_album_t *libmtpAlbum = [album toLIBMTPAlbum:track];
  libmtpAlbum->parent_id = device->default_album_folder;
	
  int playlistState = [album state];
  [album setState:NJB_PL_UNCHANGED];
		
  if (playlistState == NJB_PL_NEW)
  {
    if (LIBMTP_Create_New_Album(device, libmtpAlbum) != 0)
    {
      LIBMTP_destroy_album_t(libmtpAlbum);
      NSLog(@"Could not create new album");
      
      NSString *errorString = [self njbErrorString];
      NSLog(errorString);
      
      [albums removeObject:album];
      
      // note it will think this has succeeded since we have to way of returning error yet.
      // used to be a bug here: if we failed to make a new album, we'd keep the album in the list
      // then try to add to it.  Now we remove it :).
      
      return nil;
    }
    else
    {
      if (libmtpAlbum->album_id == 0)
      {
        NSLog(@"Created new album supposedly correctly, but got ID 0 back.. bailing out");
        [albums removeObject:album];
        return nil;
      }
        
    }
  }
  else
  {
    if (LIBMTP_Update_Album(device, libmtpAlbum) != 0)
    {
      LIBMTP_destroy_album_t(libmtpAlbum);
      NSLog(@"Could not update album");
      return nil;
    }
  }
  
  [album setAlbumID:libmtpAlbum->album_id];

  if (libmtpAlbum->album_id == 0)
    NSLog(@"the album_id is 0!");

  LIBMTP_destroy_album_t(libmtpAlbum);
  
  return album;
}

- (NJBTransactionResult *)uploadAlbumArt:(NSBitmapImageRep *)imageRep forAlbum:(Album *)album
{ 
  if (album == nil)
    return [[[NJBTransactionResult alloc] initWithSuccess:NO resultString:@"Cannot upload album art for nil album"] autorelease];
    
  if (imageRep == nil)
    return [[[NJBTransactionResult alloc] initWithSuccess:YES] autorelease];
  
  NSImage *image = [[NSImage alloc] init];
  [image addRepresentation:imageRep];
  NSSize size;
  size.width = albumArtWidth;
  size.height = albumArtHeight;
  NSImage *scaledImage = [image imageByScalingProportionallyToSize:size];
  NSDictionary *properties = [NSDictionary dictionaryWithObject:[NSNumber numberWithFloat:0.9] forKey:NSImageCompressionFactor];
  NSBitmapImageRep *bitmapImageRepScaled = [NSBitmapImageRep imageRepWithData:[scaledImage TIFFRepresentation]];
  NSData *data = [bitmapImageRepScaled representationUsingType:NSJPEGFileType properties:properties];
  if (data == nil)
  {
    NSLog(@"nil data in album art");
    return [[[NJBTransactionResult alloc] initWithSuccess:YES] autorelease];
  }
  const char *bytes = [data bytes];
  
  LIBMTP_filesampledata_t sampledata;
  sampledata.data = (char *)bytes;
  sampledata.size = [data length];
  NSLog(@"Sending album art of %d bytes", sampledata.size);
  sampledata.duration = 0;
  sampledata.filetype = LIBMTP_FILETYPE_JPEG;
  sampledata.height = size.height;
  sampledata.width = size.width;
  
  int ret = LIBMTP_Send_Representative_Sample(device, [album albumID], &sampledata);
  
  [image release];
    
  if (ret != 0)
  {
    NSString *error = @"Could not send representative sample data";
    NSLog(error);
    NSString *errorString = [self njbErrorString];
    NSLog(errorString);
    return [[[NJBTransactionResult alloc] initWithSuccess:NO resultString:error extendedErrorString:errorString] autorelease];
  }
  
  return [[[NJBTransactionResult alloc] initWithSuccess:YES] autorelease];
}

- (NJBTransactionResult *)uploadAlbumArtForTrack:(Track *)track
{   
  NSString *albumName = [track album];
  
  // don't add to a blank album
  if ([albumName isEqualToString:@""])
    return [[[NJBTransactionResult alloc] initWithSuccess:NO resultString:@"Cannot create album for blank album name"] autorelease];;
  
  if (albums == nil)
    [self downloadAlbumList];
  
  Album *album;
  NSEnumerator *albumEnumerator = [albums objectEnumerator];
  
  while (album = [albumEnumerator nextObject])
  {
    if ([[album name] caseInsensitiveCompare:albumName] == NSOrderedSame)
      break;
  }
  
  // TODO: need to add @"Uploading Album Art" to localized strings
  
  if (album == nil)
  {
    album = [self addTrack:track ToAlbumWithName:albumName];
    if (album == nil)
      return [[[NJBTransactionResult alloc] initWithSuccess:NO resultString:@"Could not add track to album"] autorelease];
  }
  
  return [self uploadAlbumArt:[track image] forAlbum:album];
}

- (void)setCreateAlbumFiles:(BOOL)newCreateAlbumFiles
{
  createAlbumFiles = newCreateAlbumFiles;
}

- (void)setUploadAlbumArt:(BOOL)newUploadAlbumArt
{
  uploadAlbumArt = newUploadAlbumArt;
}

- (void)setAlbumArtWidth:(unsigned)newAlbumArtWidth
{
  albumArtWidth = newAlbumArtWidth;
}

- (void)setAlbumArtHeight:(unsigned)newAlbumArtHeight
{
  albumArtHeight = newAlbumArtHeight;
}

- (void)cancelCurrentTransaction
{
  cancelCurrentTransaction = YES;
}

- (void)initDevice
{
  [self updateDiskSpace];
	
	// enable UTF8
	if (!mtpDevice)
		NJB_Set_Unicode(NJB_UC_UTF8);
	
	// check we have reset the cache, should have been done on disconnect
	[cachedTrackList release];
	cachedTrackList = nil;
	[cachedDataFileList release];
	cachedDataFileList = nil;
	
	if (mtpDevice)
	{
		char *tempString =  LIBMTP_Get_Modelname(device);
    if (tempString != NULL)
    {
      deviceString = [NSString stringWithUTF8String:tempString];
      free(tempString);
    }

		tempString = LIBMTP_Get_Serialnumber(device);
		if (tempString != NULL)
    {
      deviceIDString = [NSString stringWithUTF8String:tempString];
      free(tempString);
    }

		tempString = LIBMTP_Get_Deviceversion(device);
    NSString *versionString = nil;
    if (tempString != NULL)
    {
      versionString = [NSString stringWithUTF8String:tempString];
      free(tempString);
    }
		
		NSArray *versions = [versionString componentsSeparatedByString:@"_"];
		if ([versions count] != 2)
		{
			deviceVersionString = versionString;
			firmwareVersionString = @"";
		}
		else
		{
			deviceVersionString = [versions objectAtIndex:1];
			firmwareVersionString = [versions objectAtIndex:0];
		}
		
		uint8_t currentLevel = 0;
		if (LIBMTP_Get_Batterylevel(device, &batteryLevelMax, &currentLevel) != 0)
		{
			NSLog(@"Could not get battery level property description");
      NSString *errorString = [self njbErrorString];
      NSLog(errorString);
      //return [[[NJBTransactionResult alloc] initWithSuccess:NO resultString:@"Could not get battery level property description" extendedErrorString:errorString] autorelease];
		}
		
		LIBMTP_Get_Supported_Filetypes(device, &supportedFiletypes, &supportedFiletypesLength);
    
    [self downloadAlbumList];
		
		// download the data files to the cache
		//[self dataFiles];
	}
	else
	{
		[self storeDeviceString];
		[self storeFirmwareVersionString];
		[self storeDeviceIDString];
		[self storeDeviceVersionString];
	}
}

@end

// this is copied (as allowed) from http://theocacao.com/document.page/498
@implementation NSImage (ProportionalScaling)

- (NSImage*)imageByScalingProportionallyToSize:(NSSize)targetSize
{
  NSImage* sourceImage = self;
  NSImage* newImage = nil;

  if ([sourceImage isValid])
  {
    NSSize imageSize = [sourceImage size];
    float width  = imageSize.width;
    float height = imageSize.height;
    
    float targetWidth  = targetSize.width;
    float targetHeight = targetSize.height;
    
    float scaleFactor  = 0.0;
    float scaledWidth  = targetWidth;
    float scaledHeight = targetHeight;
    
    NSPoint thumbnailPoint = NSZeroPoint;
    
    if ( NSEqualSizes( imageSize, targetSize ) == NO )
    {
    
      float widthFactor  = targetWidth / width;
      float heightFactor = targetHeight / height;
      
      if ( widthFactor < heightFactor )
        scaleFactor = widthFactor;
      else
        scaleFactor = heightFactor;
      
      scaledWidth  = width  * scaleFactor;
      scaledHeight = height * scaleFactor;
      
      if ( widthFactor < heightFactor )
        thumbnailPoint.y = (targetHeight - scaledHeight) * 0.5;
      
      else if ( widthFactor > heightFactor )
        thumbnailPoint.x = (targetWidth - scaledWidth) * 0.5;
    }
    
    newImage = [[NSImage alloc] initWithSize:targetSize];
    
    [newImage lockFocus];
    
      NSRect thumbnailRect;
      thumbnailRect.origin = thumbnailPoint;
      thumbnailRect.size.width = scaledWidth;
      thumbnailRect.size.height = scaledHeight;
      
      [sourceImage drawInRect: thumbnailRect
                     fromRect: NSZeroRect
                    operation: NSCompositeSourceOver
                     fraction: 1.0];
    
    [newImage unlockFocus];
  }
  
  return [newImage autorelease];
}

@end
