//
//  MyNSTextField.m
//  XNJB
//
//  Created by Richard Low on Wed Sep 01 2004.
//

/* this class extends NSTextField to make the text
 * color grey when disabled. Used for the text labels
 */

#import "MyNSTextField.h"


@implementation MyNSTextField

- (void)setEnabled:(BOOL)enabled
{
	[super setEnabled:enabled];
	if (enabled)
		[self setTextColor:[NSColor blackColor]];
	else
		[self setTextColor:[NSColor lightGrayColor]];
}

@end
