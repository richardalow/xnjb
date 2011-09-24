//
//  DuplicateTrackFinder.h
//  XNJB
//
//  Created by Richard Low on 11/01/2005.
//  Copyright 2005 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "MyBool.h"

@interface DuplicateTrackFinder : NSObject {

}
+ (NSMutableArray *)findDuplicates:(NSMutableArray *)tracks
												 lengthTol:(unsigned)lengthTol
											 filesizeTol:(unsigned)filesizeTol
													titleTol:(unsigned)titleTol
												 artistTol:(unsigned)artistTol
													 carryOn:(MyBool *)carryOn;

@end
