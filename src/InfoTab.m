//
//  InfoTab.m
//  XNJB
//
//  Created by Richard Low on 05/06/2005.
//

#import "InfoTab.h"
#import "FilesizeFormatter.h"
#import "defs.h"

// declare the private methods
@interface InfoTab (PrivateAPI)
- (void)updateBatteryStatusWithLevel:(NSNumber *)level status:(NSString *)status;
- (void)updateBatteryStatusWorker;
@end

@implementation InfoTab

- (void)awakeFromNib
{
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(updateDisplay) name:NOTIFICATION_TASK_COMPLETE object:nil];
}

- (void)dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	
	[super dealloc];
}

- (void)updateDisplay
{
	if (![theNJB isConnected])
	{
		[self clearAll];
	}
	else
	{
		[deviceName setStringValue:[theNJB deviceString]];
		[firmware setStringValue:[theNJB firmwareVersionString]];
		[deviceID setStringValue:[theNJB deviceIDString]];
		[deviceVersion setStringValue:[theNJB deviceVersionString]];
		[diskSize setStringValue:[FilesizeFormatter filesizeString:[theNJB totalDiskSpace]]];
		if ([theNJB totalDiskSpace] != 0)
			[diskLevel setDoubleValue:(1.0 - (double)[theNJB freeDiskSpace] / (double)[theNJB totalDiskSpace])];
		else
			[diskLevel setDoubleValue:0.0];
		[remainingBytes setStringValue:[FilesizeFormatter filesizeString:[theNJB freeDiskSpace]]];
		
		NJBQueueItem *updateBatteryStatus = [[NJBQueueItem alloc] initWithTarget:self withSelector:@selector(updateBatteryStatusWorker)];
		[updateBatteryStatus setDisplayStatus:NO];
		[self addToQueue:updateBatteryStatus];
		[updateBatteryStatus release];
		
		[batteryLevel setMaxValue:[theNJB batteryLevelMax]];
	}
}

- (void)clearAll
{
	[deviceName setStringValue:@""];
	[firmware setStringValue:@""];
	[deviceID setStringValue:@""];
	[deviceVersion setStringValue:@""];
	[diskSize setStringValue:@""];
	[diskLevel setDoubleValue:0.0];
	[batteryLevel setDoubleValue:0.0];
	[remainingBytes setStringValue:@""];
	[chargeStatus setStringValue:@""];
}

// get the battery status from the jukebox (must be run in worker thread)
- (void)updateBatteryStatusWorker
{
	// only do this if we're still connected - we may have been queued when connected, but by the time we get here
	// we may have disconnected
	if ([theNJB isConnected])
	{
		NSNumber *level = [[NSNumber alloc] initWithInt:[theNJB batteryLevel]];
		NSString *status = [theNJB batteryStatus];
		
		NJBQueueItem *updateBatteryStatus = [[NJBQueueItem alloc] initWithTarget:self withSelector:@selector(updateBatteryStatusWithLevel:status:)];
		[updateBatteryStatus setDisplayStatus:NO];
		[updateBatteryStatus setRunInMainThread:NO];
		[updateBatteryStatus setObject1:level];
		[updateBatteryStatus setObject2:status];
		
		[self addToQueue:updateBatteryStatus];
		
		[updateBatteryStatus release];
		[level release];
	}
}

- (void)updateBatteryStatusWithLevel:(NSNumber *)level status:(NSString *)status
{
	[batteryLevel setDoubleValue:[level doubleValue]];
	[chargeStatus setStringValue:status];
}

- (void)activate
{
	[super activate];
	[self updateDisplay];
}

- (void)onConnectAndActive
{
	[self updateDisplay];
}

@end
