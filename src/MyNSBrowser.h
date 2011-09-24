//
//  MyNSBrowser.h
//  XNJB
//
//  Created by Richard Low on 01/07/2005.
//

#import <Cocoa/Cocoa.h>


@interface MyNSBrowser : NSBrowser {

}
- (NSArray *)pathComponents;
- (NSArray *)pathComponentsToColumn:(int)column;
- (NSArray *)containingPathComponents;
- (NSString *)containingPath;

@end
