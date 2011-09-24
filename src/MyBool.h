//
//  MyBool.h
//  XNJB
//
//  Created by Richard Low on 01/04/2005.
//  Copyright 2005 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>


@interface MyBool : NSObject {
	BOOL value;
}
- (id)initWithValue:(BOOL) newValue;
- (void)setValue:(BOOL) newValue;
- (BOOL)value;

@end
