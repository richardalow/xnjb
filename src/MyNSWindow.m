//
//  MyNSWindow.m
//  XNJB
//
//  Created by Richard Low on Thu Jul 22 2004.
//

/* an NSWindow subclass for the main window
 * that tells target when the first responder
 * changes. Sets the frameAutosaveName to 
 * PREF_KEY_MAIN_WINDOW_FRAME
 */

#import "MyNSWindow.h"
#import "defs.h"

@implementation MyNSWindow

- (BOOL)makeFirstResponder:(NSResponder *)aResponder
{
	if (target != nil && [target respondsToSelector:selector])
		[target performSelector:selector withObject:aResponder];
	
	return [super makeFirstResponder:aResponder];
}

- (void)setFirstResponderChangeTarget:(id)newTarget
{
	target = newTarget;
}

- (void)setFirstResponderChangeSelector:(SEL)newSelector
{
	selector = newSelector;
}

- (void)awakeFromNib
{
	[self setFrameAutosaveName:PREF_KEY_MAIN_WINDOW_FRAME];
}

@end
