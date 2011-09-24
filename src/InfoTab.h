//
//  InfoTab.h
//  XNJB
//
//  Created by Richard Low on 05/06/2005.
//

#import <Cocoa/Cocoa.h>
#import "MyTab.h"

@interface InfoTab : MyTab {
  IBOutlet NSTextField *deviceName;
	IBOutlet NSTextField *firmware;
	IBOutlet NSTextField *deviceID;
	IBOutlet NSTextField *diskSize;
	IBOutlet NSTextField *deviceVersion;
	IBOutlet NSTextField *chargeStatus;
	IBOutlet NSTextField *remainingBytes;
	IBOutlet NSProgressIndicator *batteryLevel;
	IBOutlet NSProgressIndicator *diskLevel;
}
- (void)updateDisplay;
- (void)clearAll;

@end
