//
//  WMATagger.h
//  XNJB
//
//  Created by Richard Low on 17/09/2004.
//  Copyright 2004 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "Track.h"

@interface WMATagger : NSObject {

}
+ (Track *)readTrack:(NSString *)filename;

@end
