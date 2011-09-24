//
//  MenuController.m
//  XNJB
//

/* this class responds to all menu clicks
 * and queries for whether we can click them or not
 * Forwards the messages on to the correct classes
 */
#import "MenuController.h"

@implementation MenuController

// init/dealloc methods

- (void)dealloc
{
	[preferencesController release];
	[super dealloc];
}

/**********************/

- (IBAction)copyToComputer:(id)sender
{
	if (![[mainController currentTab] respondsToSelector:@selector(copyFromNJB:)])
	{
		NSLog(@"copyToComputer when not in music/data tab!");
		return;
	}
	[[mainController currentTab] copyFromNJB:nil];
}

- (IBAction)copyToNJB:(id)sender
{
	if (![[mainController currentTab] respondsToSelector:@selector(copyToNJB:)])
	{
		NSLog(@"copyToComputer when not in music/data tab!");
		return;
	}
	[[mainController currentTab] copyToNJB:nil];
}

- (IBAction)delete:(id)sender
{
	MyTab *currentTab = [mainController currentTab];
	if (currentTab == [mainController musicTab])
		[[mainController musicTab] menuDelete];
	else if (currentTab == [mainController dataTab])
		[[mainController dataTab] menuDelete];
	else if (currentTab == [mainController duplicateItemsTab])
		[[mainController duplicateItemsTab] menuDelete];
}

- (IBAction)connect:(id)sender
{
	[mainController menuConnect];
}

- (IBAction)disconnect:(id)sender
{
	[mainController menuDisconnect];
}

- (IBAction)cancelQueuedItem:(id)sender
{
	[[mainController queueTab] menuCancelQueuedItem];
}

/* load the preferences window - only if
 * not already loaded.
 * Then show it
 */
- (IBAction)preferences:(id)sender
{
	if (!preferencesController)
	{
		preferencesController = [[PreferencesWindowController alloc] init];
		[preferencesController setPreferences:preferences];
		[preferencesController setTabView:tabView];
	}
	[preferencesController showWindow:self];
}

/* should menuItem be enabled now?
 * Uses the selectors to identify the menu items
 */
- (BOOL)validateMenuItem:(NSMenuItem *)menuItem
{
	id currentTab = [mainController currentTab];
	if ([menuItem action] == @selector(delete:))
	{
		if (currentTab == [mainController musicTab])
			return [[mainController musicTab] menuShouldDelete];
		else if (currentTab == [mainController dataTab])
			return [[mainController dataTab] menuShouldDelete];
		else if (currentTab == [mainController duplicateItemsTab])
			return [[mainController duplicateItemsTab] menuShouldDelete];
		else
			return NO;
	}
	else if ([menuItem action] == @selector(connect:))
	{
		return [mainController menuShouldConnect];
	}
	else if ([menuItem action] == @selector(disconnect:))
	{
		return [mainController menuShouldDisconnect];
	}
	else if ([menuItem action] == @selector(cancelQueuedItem:))
	{
		if (currentTab != [mainController queueTab])
			return NO;
		else
			return [[mainController queueTab] menuShouldCancelQueuedItem];
	}
	else if ([menuItem action] == @selector(copyToNJB:))
	{
		if (currentTab == [mainController musicTab] || currentTab == [mainController dataTab])
			return [currentTab canCopyToNJB];
		else
			return NO;
	}
	else if ([menuItem action] == @selector(copyToComputer:))
	{
		if (currentTab == [mainController musicTab] || currentTab == [mainController dataTab])
			return [currentTab canCopyFromNJB];
		else
			return NO;		
	}
	else if ([menuItem action] == @selector(preferences:))
	{
		return YES;
	}
	else
		return NO;
}

@end
