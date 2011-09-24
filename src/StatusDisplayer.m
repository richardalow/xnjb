//
//  StatusDisplayer.m
//  XNJB
//
//  Created by Richard Low on Sat Jul 24 2004.
//

/* this class updates the status in the status text box, 
 * main progress bar and queue tab progress bar.
 * Methods called by NJBQueueConsumer for start/stop tasks
 * and called by NJB for progress updates.
 * Posts notifications when tasks start/stop
 */
#import "StatusDisplayer.h"
#import "NJBQueueItem.h"
#import "defs.h"
#import "FilesizeFormatter.h"

// declare the private methods
@interface StatusDisplayer (PrivateAPI)
- (void)displayTaskProgress:(NSNumber *)progress;
- (void)displayTotalProgress:(NSNumber *)progress;
- (void)updateTotalProgress:(double)newProgress;
- (NSString *)idleString;
@end

@implementation StatusDisplayer

// init/dealloc methods

- (id)init
{
	if (self = [super init])
	{
		NSNotificationCenter *nc;
		nc = [NSNotificationCenter defaultCenter];
		[nc addObserver:self selector:@selector(NJBConnected:) name:NOTIFICATION_CONNECTED object:nil];
		[nc addObserver:self selector:@selector(NJBDisconnected:) name:NOTIFICATION_DISCONNECTED object:nil];
		
		// I know I am intialized in the main thread when initialized from NIB
		mainThread = [[NSThread currentThread] retain];
		
		// I know I'm disconnected when start
		[self setStatus:STATUS_DISCONNECTED];
	}
	return self;
}

- (void)dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	[mainThread release];
	[mainThreadProxy release];
	[idleString release];
	[mainThreadQueue release];
	[mainThreadQueueLock release];
	[super dealloc];
}

// accessor methods

- (void)setMainThreadQueue:(Queue *)newMainThreadQueue
{
	[newMainThreadQueue retain];
	[mainThreadQueue release];
	mainThreadQueue = newMainThreadQueue;
}

- (void)setMainThreadQueueLock:(NSLock *)newMainThreadQueueLock
{
	[newMainThreadQueueLock retain];
	[mainThreadQueueLock release];
	mainThreadQueueLock = newMainThreadQueueLock;
}


/**********************/

/* called by NJBQueueConsumer when a task is complete
 */
- (void)taskComplete:(NJBTransactionResult *)result
{
	// increase counter for total bar, gets percentage from taskCount
	tasksComplete++;
		
	NSString *totalSpace = [FilesizeFormatter filesizeString:totalDiskSpace];
	NSString *usedSpace = [FilesizeFormatter filesizeString:(totalDiskSpace - freeDiskSpace)];
	
	[self runInMainThread:diskSpaceText withSelector:@selector(setStringValue:) withObject:[NSString stringWithFormat:@"%@ / %@", usedSpace, totalSpace]];
	
	if (taskCount == 0)
	{
		NSLog(@"taskCount == 0 in taskComplete!");
	}
	else
	{
		[self updateTotalProgress:100.0*(double)tasksComplete/(double)taskCount];
	}
	
	[self postNotificationName:NOTIFICATION_TASK_COMPLETE object:result];
}

- (void)updateTaskProgress:(double)newProgress
{
	[self runInMainThread:self withSelector:@selector(displayTaskProgress:) withObject:[NSNumber numberWithDouble:newProgress]];
	
	if (taskCount == 0)
	{
		NSLog(@"taskCount == 0 in updateTaskProgress! (this is normal if sent setIdle when was idle)");
		return;
	}
	double totalProgress = (newProgress + 100.0 * (double)tasksComplete) / (double)taskCount;
	if (totalProgress > 100.0)
	{
		NSLog(@"totalProgress > 100.0 in updateTaskProgress");
		return;
	}
	[self updateTotalProgress:totalProgress];
}

- (void)updateTotalProgress:(double)newProgress
{
	[self runInMainThread:self withSelector:@selector(displayTotalProgress:) withObject:[NSNumber numberWithDouble:newProgress]];
}

/* call me when a new task is added
 */
- (void)incrementTaskCount
{
	taskCount++;
}

- (NSString *)statusStringForStatus:(statusTypes)status
{
	switch (status)
	{
		case STATUS_CONNECTED:
			return [NSString stringWithString:NSLocalizedString(@"Connected", nil)];
		case STATUS_DISCONNECTED:
			return [NSString stringWithString:NSLocalizedString(@"Disconnected", nil)];
		case STATUS_NO_NJB:
			return [NSString stringWithString:NSLocalizedString(@"No NJB Devices Found", nil)];
		case STATUS_IDLE:
			// return a copy of the idle string: this will get set back to the idle string later releasing the old one!
			return [NSString stringWithString:[self idleString]];
		case STATUS_COULD_NOT_OPEN:
			return [NSString stringWithString:NSLocalizedString(@"Could Not Open NJB", nil)];
		case STATUS_COULD_NOT_CAPTURE:
			return [NSString stringWithString:NSLocalizedString(@"Could Not Capture NJB", nil)];
		case STATUS_UPLOADING_TRACK:
			return [NSString stringWithString:NSLocalizedString(@"Uploading Track", nil)];
		case STATUS_UPLOADING_FILE:
			return [NSString stringWithString:NSLocalizedString(@"Uploading File", nil)];
		case STATUS_DOWNLOADING_TRACK_LIST:
			return [NSString stringWithString:NSLocalizedString(@"Downloading Track List", nil)];
		case STATUS_DOWNLOADING_FILE_LIST:
			return [NSString stringWithString:NSLocalizedString(@"Downloading File List", nil)];
		case STATUS_UPDATING_TRACK_TAG:
			return [NSString stringWithString:NSLocalizedString(@"Updating Track Tag", nil)];
		case STATUS_DOWNLOADING_TRACK:
			return [NSString stringWithString:NSLocalizedString(@"Downloading Track", nil)];
		case STATUS_DELETING_TRACK:
			return [NSString stringWithString:NSLocalizedString(@"Deleting Track", nil)];
		case STATUS_DOWNLOADING_FILE:
			return [NSString stringWithString:NSLocalizedString(@"Downloading File", nil)];
		case STATUS_CONNECTING:
			return [NSString stringWithString:NSLocalizedString(@"Connecting", nil)];
		case STATUS_DISCONNECTING:
			return [NSString stringWithString:NSLocalizedString(@"Disconnecting", nil)];
		case STATUS_DELETING_FILE:
			return [NSString stringWithString:NSLocalizedString(@"Deleting File", nil)];
		case STATUS_DOWNLOADING_SETTINGS:
			return [NSString stringWithString:NSLocalizedString(@"Downloading Jukebox Settings", nil)];
		case STATUS_UPDATING_SETTINGS:
			return [NSString stringWithString:NSLocalizedString(@"Updating Jukebox Settings", nil)];
		case STATUS_DOWNLOADING_PLAYLISTS:
			return [NSString stringWithString:NSLocalizedString(@"Downloading Playlists", nil)];
		case STATUS_UPDATING_PLAYLIST:
			return [NSString stringWithString:NSLocalizedString(@"Updating Playlist", nil)];
		case STATUS_DELETING_PLAYLIST:
			return [NSString stringWithString:NSLocalizedString(@"Deleting Playlist", nil)];
		case STATUS_CREATING_FOLDER:
			return [NSString stringWithString:NSLocalizedString(@"Creating Folder", nil)];
    case STATUS_UPLOADING_ALBUM_ART:
      return [NSString stringWithString:NSLocalizedString(@"Uploading Album Art", nil)];
		case STATUS_UNKNOWN:
		default:
			NSLog(@"Unknown status %d in StatusDisplayer.statusStringForStatus", status);
			return [NSString stringWithString:NSLocalizedString(@"Unknown Status", nil)];
	}
}

- (void)setStatus:(statusTypes)newStatus
{
	NSString *statusString = [self statusStringForStatus:newStatus];
	switch (newStatus)
	{
		case STATUS_CONNECTED:
		case STATUS_DISCONNECTED:
		case STATUS_NO_NJB:
		case STATUS_IDLE:
		case STATUS_COULD_NOT_OPEN:
		case STATUS_COULD_NOT_CAPTURE:
			[self setIdleString:statusString];
		default:
			break;
	}	
	[self runInMainThread:statusTextField withSelector:@selector(setStringValue:) withObject:statusString];
}

/* call me when a task is starting
 */
- (void)startTask
{
	[self postNotificationName:NOTIFICATION_TASK_STARTING object:nil];
}

/* called when no tasks remain
 */
- (void)setIdle
{
	[self setStatus:STATUS_IDLE];
	[self updateTaskProgress:0.0];
	[self updateTotalProgress:0.0];
	// should be zero anyway
	taskCount = 0;
	tasksComplete = 0;
}

- (BOOL)isIdle
{
	return (taskCount == 0);
}

/* set a proxy object to the main thread so I can 
 * update UI items in a thread safe way
 */
- (void)setMainThreadProxy:(id)newMainThreadProxy
{
	[newMainThreadProxy retain];
	[mainThreadProxy release];
	mainThreadProxy = newMainThreadProxy;
}

// must be run in main thread only
- (void)displayTaskProgress:(NSNumber *)progress
{
	[taskProgressIndicator setDoubleValue:[progress doubleValue]];
}

// must be run in main thread only
- (void)displayTotalProgress:(NSNumber *)progress
{
	[totalProgressIndicator setDoubleValue:[progress doubleValue]];
}

- (void)setIdleString:(NSString *)newIdleString
{
	if (newIdleString == nil)
		newIdleString = @"";
	// get immutable copy
	newIdleString = [NSString stringWithString:newIdleString];
	[newIdleString retain];
	[idleString release];
	idleString = newIdleString;
	return;
}

- (NSString *)idleString
{
	if (idleString == nil)
		[self setIdleString:[NSString stringWithString:@""]];
	return idleString;
}

- (void)runInMainThread:(id)target withSelector:(SEL)selector withObject:(id)object
{
	[self runInMainThread:target withSelector:selector withObject:object withObject:nil];
}

/* send the message selector to target in the main thread
 * checks to see if we are in the main thread - 
 * if not add message to the main thread queue for processing
 * there.
 */
- (void)runInMainThread:(id)target withSelector:(SEL)selector withObject:(id)object1 withObject:(id)object2
{
	if ([NSThread currentThread] == mainThread)
	{
		[target performSelector:selector withObject:object1 withObject:object2];
	}
	else
	{
		NJBQueueItem *queueItem = [[NJBQueueItem alloc] initWithTarget:target withSelector:selector withObject:object1];
		[queueItem setDisplayStatus:NO];
		[queueItem setObject2:object2];
		[mainThreadQueueLock lock];
		[mainThreadQueue addObject:queueItem];
		[mainThreadQueueLock unlock];
		[queueItem release];
		// a new autorelease pool is required: there is some
		// limit to the number of autoreleased object that makes
		// this proxy call crash after 65530 calls.
		// e.g. this code will crash on the 65531st attempt
		/*
		 int i = 0;
		 for (i = 0; i < 65535; i++)
		 {
			 [mainThreadProxy consumeQueue];
		 }
		 */
		NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
		[mainThreadProxy consumeQueue];
		[pool release];
	}
}

/* post a notification in the main thread
 */
- (void)postNotificationName:(NSString *)name object:(id)object
{
	if ([NSThread currentThread] == mainThread)
	{
		NSNotificationCenter *nc;
		nc = [NSNotificationCenter defaultCenter];
		[nc postNotificationName:name object:object];
	}
	else
		[mainThreadProxy postNotificationName:name object:object];
}

- (void)NJBDisconnected:(NSNotification *)note
{
	[self setStatus:STATUS_DISCONNECTED];
}

- (void)NJBConnected:(NSNotification *)note
{
	[self setStatus:STATUS_CONNECTED];
}

/* count tasks have been cancelled
 */
- (void)cancelTasks:(unsigned)count
{
	taskCount-= count;
}

/* called when disk space has changed by NJB class
 */
- (void)updateDiskSpace:(unsigned long long)newTotalSpace withFreeSpace:(unsigned long long)newFreeSpace
{
	totalDiskSpace = newTotalSpace;
	freeDiskSpace = newFreeSpace;
}

@end
