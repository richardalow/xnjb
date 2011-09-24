//
//  StatusDisplayer.h
//  XNJB
//
//  Created by Richard Low on Sat Jul 24 2004.
//

#import <Foundation/Foundation.h>
#import "Queue.h"
#import "NJBTransactionResult.h"

/* the values for each status are completely arbitrary... just added as I thought of them
 */
typedef enum _statusTypes {STATUS_CONNECTED = 0, STATUS_DISCONNECTED = 1, STATUS_UPLOADING_TRACK = 2,
													 STATUS_UPLOADING_FILE = 3, STATUS_NO_NJB = 4, STATUS_DOWNLOADING_TRACK_LIST = 5,
													 STATUS_IDLE = 6, STATUS_DOWNLOADING_FILE_LIST = 7, STATUS_UPDATING_TRACK_TAG = 8,
													 STATUS_COULD_NOT_OPEN = 9, STATUS_COULD_NOT_CAPTURE = 10, STATUS_DOWNLOADING_TRACK = 11,
													 STATUS_DOWNLOADING_FILE = 12, STATUS_CONNECTING = 13, STATUS_UNKNOWN = 14,
													 STATUS_DELETING_TRACK = 15, STATUS_DISCONNECTING = 16, STATUS_DELETING_FILE = 17,
													 STATUS_DOWNLOADING_SETTINGS = 18, STATUS_UPDATING_SETTINGS = 19,
													 STATUS_DOWNLOADING_PLAYLISTS = 20, STATUS_UPDATING_PLAYLIST = 21, STATUS_DELETING_PLAYLIST = 22,
													 STATUS_CREATING_FOLDER = 23, STATUS_UPLOADING_ALBUM_ART = 24} statusTypes;

@interface StatusDisplayer : NSObject {
	IBOutlet NSProgressIndicator *taskProgressIndicator;
	IBOutlet NSProgressIndicator *totalProgressIndicator;
	IBOutlet NSTextField *statusTextField;
	NSThread *mainThread;
	id mainThreadProxy;
	Queue *mainThreadQueue;
	NSLock *mainThreadQueueLock;
	NSString *idleString;
	unsigned taskCount;
	unsigned tasksComplete;
	IBOutlet NSTextField *diskSpaceText;
	unsigned long long totalDiskSpace;
	unsigned long long freeDiskSpace;
}

- (void)updateDiskSpace:(unsigned long long)newTotalSpace withFreeSpace:(unsigned long long)newFreeSpace;
- (void)updateTaskProgress:(double)newProgress;
- (void)postNotificationName:(NSString *)name object:(id)object;
- (void)taskComplete:(NJBTransactionResult *)result;
- (void)incrementTaskCount;
- (NSString *)statusStringForStatus:(statusTypes)status;
- (void)setStatus:(statusTypes)newStatus;
- (void)setMainThreadProxy:(id)mainThreadProxy;
- (void)startTask;
- (void)setIdle;
- (void)setIdleString:(NSString *)newIdleString;
- (void)runInMainThread:(id)target withSelector:(SEL)selector withObject:(id)object;
- (void)runInMainThread:(id)target withSelector:(SEL)selector withObject:(id)object1 withObject:(id)object2;
- (void)setMainThreadQueue:(Queue *)newMainThreadQueue;
- (void)setMainThreadQueueLock:(NSLock *)newMainThreadQueueLock;
- (void)NJBDisconnected:(NSNotification *)note;
- (void)NJBConnected:(NSNotification *)note;
- (void)cancelTasks:(unsigned)count;
- (BOOL)isIdle;
@end
