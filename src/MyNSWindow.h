//
//  MyNSWindow.h
//  XNJB
//
//  Created by Richard Low on Thu Jul 22 2004.
//

#import <Foundation/Foundation.h>


@interface MyNSWindow : NSWindow {
	id target;
	SEL selector;
}
- (void)setFirstResponderChangeTarget:(id)newTarget;
- (void)setFirstResponderChangeSelector:(SEL)newSelector;

@end
