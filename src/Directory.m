//
//  Directory.m
//  XNJB
//
//  Created by Richard Low on 01/07/2005.
//

#import "Directory.h"

@interface NSArray (itemWithID)
- (MyItem *)itemWithID:(unsigned)itemID;
- (MyItem *)itemWithDescription:(NSString *)desc;
@end

@implementation NSArray (itemWithID)

/* finds an item which has ID itemID
 * assumes all IDs are unique so returns first item only
 * returns nil if none found
 */
- (MyItem *)itemWithID:(unsigned)itemID
{
	NSEnumerator *enumerator = [self objectEnumerator];
	MyItem *currentItem;
	while (currentItem = [enumerator nextObject])
	{
		if ([currentItem itemID] == itemID)
			return currentItem;
	}
	return nil;
}

/* finds an item which has description desc
 * assumes all descriptions are unique so returns first item only
 * returns nil if none found
 */
- (MyItem *)itemWithDescription:(NSString *)desc
{
	NSEnumerator *enumerator = [self objectEnumerator];
	MyItem *currentItem;
	while (currentItem = [enumerator nextObject])
	{
		if ([[currentItem description] isEqualToString:desc])
			return currentItem;
	}
	return nil;
}

@end

// declare the private methods
@interface Directory (PrivateAPI)
- (NSArray *)arrayWithFirstObjectRemoved:(NSArray *)array;
- (void)removeItemWithDescription:(NSString *)desc;
- (void)insertItem:(id)item;
- (NSMutableDictionary *)flatten:(NSMutableDictionary *)dictSoFar;
@end

@implementation Directory

// init/dealloc methods

- (id)init
{
	return [self initWithName:@""];
}

- (Directory *)initWithName:(NSString *)newName
{
	return [self initWithName:newName withContents:nil];
}

// makes a copy of newContents
- (Directory *)initWithName:(NSString *)newName withContents:(NSMutableArray *)newContents
{
	if (self = [super init])
	{
		[self setName:newName];
		contents = [[NSMutableArray alloc] initWithArray:newContents];
	}
	return self;
}

- (void)dealloc
{
	[contents release];
	[name release];
	[super dealloc];
}

// accessor methods

- (NSString *)name
{
	return name;
}

- (void)setName:(NSString *)newName
{
	if (newName == nil)
		newName = @"";
	// get immutable copy
	newName = [NSString stringWithString:newName];
	[newName retain];
	[name release];
	name = newName;
}

/**********************/

// quasi accessor methods

/* we use descriptions to identify the items.
 * Use the name here.
 */
- (NSString *)description
{
	return name;
}

/* return the number of items (files & dirs)
 * in this directory.
 */
- (unsigned int)itemCount
{
	return [contents count];
}

/* return the item at the index index.
 * Useful for NSBrowser delegate functions.
 */
- (id)itemAtIndex:(unsigned int)index
{
	return [contents objectAtIndex:index];
}

/* returns the object enumerator for this
 * dir.
 */
- (NSEnumerator *)objectEnumerator
{
	return [contents objectEnumerator];
}

// find items by path

/* return the item with path component array
 * pathArray.  nil if not found.
 */
- (id)itemWithPath:(NSArray *)pathArray
{
	if ([pathArray count] == 0)
		return self;
	if ([pathArray count] == 1)
		return [contents itemWithDescription:[pathArray objectAtIndex:0]];
	Directory *dir = (Directory *)[contents itemWithDescription:[pathArray objectAtIndex:0]];
	return [dir itemWithPath:[self arrayWithFirstObjectRemoved:pathArray]];
}

// remove items

/* removes the item at path components array
 * pathArray.  Will fail if pathArray is empty
 * or nil.
 */
- (void)removeItemWithPath:(NSArray *)pathArray
{
	if ([pathArray count] == 0)
	{
		NSLog(@"pathArray count 0 in removeItemWithPath:");
		return;
	}
	if ([pathArray count] == 1)
	{
		[self removeItemWithDescription:[pathArray objectAtIndex:0]];
		return;
	}
	
	Directory *dir = (Directory *)[contents itemWithDescription:[pathArray objectAtIndex:0]];
	[dir removeItemWithPath:[self arrayWithFirstObjectRemoved:pathArray]];
}

/* removes at most one occurence of item with
 * description desc in this directory.
 */
- (void)removeItemWithDescription:(NSString *)desc
{
	id obj = [contents itemWithDescription:desc];
	[contents removeObject:obj];
}

/* remove the item item from the current
 * dir.
 */
- (void)removeItem:(MyItem *)item
{
	[contents removeObject:item];
}

// add items

/* add a new directory at path component array
 * path array.  If it already exists, will not
 * do anything.  Returns the new (or old) directory.
 */
- (Directory *)addNewDirectory:(NSArray *)pathArray
{
  if ([pathArray count] == 0)
		return self;
	id dir = [contents itemWithDescription:[pathArray objectAtIndex:0]];
	if (dir == nil || ![dir isMemberOfClass:[Directory class]])
	{
		dir = [[Directory alloc] initWithName:[pathArray objectAtIndex:0]];
		[self insertItem:dir];
		[dir release];
	}
	return [dir addNewDirectory:[self arrayWithFirstObjectRemoved:pathArray]];
}

// adds to this directory - see addItem:toDir:
- (Directory *)addItem:(MyItem *)file
{
	return [self addItem:file toDir:[NSArray array]];
}

// returns the directory the item was added to, and creates it if necessary
- (Directory *)addItem:(MyItem *)item toDir:(NSArray *)pathArray
{
	Directory *dir = [self addNewDirectory:pathArray];
	[dir insertItem:item];
	return dir;
}

/* private method to insert the item item
 * into this directory in the correct place.
 * This keeps sorting alphabetical.
 */
- (void)insertItem:(id)item
{
	NSEnumerator *enumerator = [contents objectEnumerator];
	id obj;
	NSComparisonResult comp;
	NSString *itemDesc = [item description];
	unsigned int index = 0;
	while (obj = [enumerator nextObject])
	{
		comp = [itemDesc caseInsensitiveCompare:[obj description]];
		// see if we've found where to put this
		if (comp != NSOrderedDescending)
			break;
		index++;
	}
	[contents insertObject:item atIndex:index];
}

// misc

/* debug routine: prints the directory structure.
 * The filenames have prefix prefix.  Generally when
 * calling this prefix is the empty string, this is used
 * when called recursively to build up paths.
 */
- (void)print:(NSString *)prefix
{
	NSLog(@"%@%@", prefix, name);
	NSEnumerator *enumerator = [contents objectEnumerator];
	id obj;
	while (obj = [enumerator nextObject])
	{
		if ([obj isMemberOfClass:[Directory class]])
		{
			[obj print:[NSString stringWithFormat:@"%@%@/", prefix, [self name]]];
		}
		else
		{
			NSLog(@"%@%@/%@", prefix, [self name], [obj filename]);
		}
	}	
}

/* private method to remove the first object from
 * array array.
 */
- (NSArray *)arrayWithFirstObjectRemoved:(NSArray *)array
{
	NSMutableArray *newArray = [NSMutableArray arrayWithArray:array];
	[newArray removeObjectAtIndex:0];
	return newArray;
}

/* do we have item with description itemName?
 */
- (BOOL)containsItemWithName:(NSString *)itemName
{
	id item = [contents itemWithDescription:itemName];
	return !(item == nil);
}

/* find the item with description desc.
 * returns nil if not found.
 */
- (MyItem *)itemWithDescription:(NSString *)desc
{
	return [contents itemWithDescription:desc];
}

- (Directory *)copy
{
//	Directory *copy = [[Directory alloc] initWithName:name withContents:contents];
	
	// pretending to be immutable for now...
	return [self retain];
	
//	return [copy autorelease];
}

- (Directory *)subdirWithID:(unsigned)dirID
{
	if ([self itemID] == dirID)
		return self;
	
	NSEnumerator *enumerator = [self objectEnumerator];
	MyItem *currentItem;
	while (currentItem = [enumerator nextObject])
	{
		if ([currentItem isMemberOfClass:[Directory class]])
		{
			Directory *dir = [currentItem subdirWithID:dirID];
			if (dir != nil)
				return dir;
		}
	}
	return nil;
}

- (NSDictionary *)flatten
{
	NSMutableDictionary *dict = [self flatten:[[[NSMutableDictionary alloc] init] autorelease]];
	// make unmutable
  return [NSDictionary dictionaryWithDictionary:dict];
}

- (NSMutableDictionary *)flatten:(NSMutableDictionary *)dictSoFar
{
	[dictSoFar setObject:self forKey:[NSNumber numberWithUnsignedInt:[self itemID]]];
	
	NSEnumerator *enumerator = [self objectEnumerator];
	MyItem *currentItem;
	while (currentItem = [enumerator nextObject])
	{
		if ([currentItem isMemberOfClass:[Directory class]])
		{
			[currentItem flatten:dictSoFar];
		}
	}
	return dictSoFar;
}

// removes empty entries at the beginning/end of the array
// that come from leading/trailing separators on paths from componentsSeparatedByString
+ (NSArray *)normalizePathArray:(NSArray *)pathArray
{
	if ([pathArray count] == 0)
		return pathArray;
	
	NSMutableArray *newPathArray = [NSMutableArray arrayWithArray:pathArray];
	if ([[newPathArray objectAtIndex:[newPathArray count] - 1] length] == 0)
		[newPathArray removeLastObject];
	
	if ([pathArray count] > 0)
		if ([[newPathArray objectAtIndex:0] length] == 0)
			[newPathArray removeObjectAtIndex:0];
	
	return [NSArray arrayWithArray:newPathArray];
}

// returns a path string with '\' as separators with leading and trailing '\'
+ (NSString *)stringFromPathArray:(NSArray *)pathArray
{
	NSMutableString *path = [[NSMutableString alloc] init];
	NSEnumerator *enumerator = [pathArray objectEnumerator];
	NSString *pathItem;
	while (pathItem = [enumerator nextObject])
	{
		if ([pathItem length] > 0)
			[path appendFormat:@"\\%@", pathItem];
	}
	[path appendString:@"\\"];
	NSString *pathImmutable = [NSString stringWithString:path];
	[path release];
	return pathImmutable;
}

@end
