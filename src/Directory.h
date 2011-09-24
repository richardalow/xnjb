//
//  Directory.h
//  XNJB
//
//  Created by Richard Low on 01/07/2005.
//

#import <Cocoa/Cocoa.h>
#import "DataFile.h"
#import "MyItem.h"

@interface Directory : MyItem {
  NSMutableArray *contents;
	NSString *name;
}
- (Directory *)initWithName:(NSString *)newName;
- (Directory *)initWithName:(NSString *)newName withContents:(NSMutableArray *)newContents;
- (NSString *)name;
- (void)setName:(NSString *)newName;
- (void)removeItemWithPath:(NSArray *)pathArray;
- (Directory *)addNewDirectory:(NSArray *)pathArray;
- (Directory *)addItem:(MyItem *)file toDir:(NSArray *)pathArray;
- (Directory *)addItem:(MyItem *)file;
- (void)print:(NSString *)prefix;
- (unsigned int)itemCount;
- (id)itemAtIndex:(unsigned int)index;
- (id)itemWithPath:(NSArray *)pathArray;
- (BOOL)containsItemWithName:(NSString *)itemName;
- (MyItem *)itemWithDescription:(NSString *)desc;
- (void)removeItem:(MyItem *)item;
- (NSEnumerator *)objectEnumerator;
- (Directory *)copy;
- (Directory *)subdirWithID:(unsigned)itemID;
- (NSDictionary *)flatten;
+ (NSArray *)normalizePathArray:(NSArray *)pathArray;
+ (NSString *)stringFromPathArray:(NSArray *)pathArray;

@end
