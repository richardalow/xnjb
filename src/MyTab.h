//
//  MyTab.h
//  XNJB
//
//  Created by Richard Low on Fri Jul 16 2004.
//

#import <Foundation/Foundation.h>
#import "DrawerController.h"
#import "NJBQueueConsumer.h"
#import "NJBQueueItem.h"
#import "NJB.h"
#import "Preferences.h"

@interface MyTab : NSObject {
	IBOutlet Preferences *preferences;
	IBOutlet NJB *theNJB;
	IBOutlet MyTab *queueTab;
  IBOutlet DrawerController *drawerController;
	NJBQueueConsumer *njbQueue;
	BOOL isActive;
	BOOL hasActivatedSinceConnect;
}
- (void)activate;
- (void)deactivate;
- (void)setNJBQueueConsumer:(NJBQueueConsumer *)newNjbQueue;
- (void)addToQueue:(NJBQueueItem *)item;
- (void)addToQueue:(NJBQueueItem *)item withSubItems:(NSArray *)subItems;
- (void)addToQueue:(NJBQueueItem *)item withSubItems:(NSArray *)subItems withDescription:(NSString *)description;
- (void)NJBConnected:(NSNotification *)note;
- (void)NJBDisconnected:(NSNotification *)note;
- (void)onConnectAndActive;
- (void)NJBTrackListModified:(NSNotification *)note;

@end
