//
//  ID3Tagger.h
//  id3libtest
//
//  Created by Richard Low on Mon Jul 26 2004.
//

#import <Foundation/Foundation.h>
#import "Track.h"

@interface ID3Tagger : NSObject {
@private
  NSString *filename;
}
- (id)initWithFilename:(NSString *)newFilename;
- (void)setFilename:(NSString *)newFilename;
- (Track *)readTrack;
- (void)writeTrack:(Track *)track;

@end
