//
//  MyNSArrayController.h
//  XNJB
//
//  Created by Richard Low on 16/04/2005.
//

#import <Cocoa/Cocoa.h>


@interface MyNSArrayController : NSArrayController {
  NSString *searchString;
}

- (IBAction)search:(id)sender;
- (void)setSearchString:(NSString *) newSearchString;
- (NSString *)searchString;
- (void)replaceObject:(id)old withObject:(id)new;
- (void)removeAll;

@end
