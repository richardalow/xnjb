//
//  MyNSArrayController.m
//  XNJB
//
//  Created by Richard Low on 16/04/2005.
//

#import "MyNSArrayController.h"


@implementation MyNSArrayController

- (void)dealloc
{
	[searchString release];
	[super dealloc];
}

- (void)setSearchString:(NSString *) newSearchString
{
	newSearchString = [newSearchString copy];
	[searchString release];
	searchString = newSearchString;
}

- (NSString *)searchString
{
	return searchString;
}

- (IBAction)search:(id)sender
{
	[self setSearchString:[sender stringValue]];
	[self rearrangeObjects];    
}

- (NSArray *)arrangeObjects:(NSArray *)objects
{
	if (searchString == nil || [searchString isEqualToString:@""])
	{
		return [super arrangeObjects:objects];   
	}
	
	NSMutableArray *matchedObjects = [NSMutableArray arrayWithCapacity:[objects count]];
	
	NSEnumerator *oEnum = [objects objectEnumerator];
	id item;	
	while (item = [oEnum nextObject])
	{
		if ([item matchesString:searchString])
			[matchedObjects addObject:item];
	}
	return [super arrangeObjects:matchedObjects];
}

- (void)replaceObject:(id)old withObject:(id)new
{
	// preserve selection if it was selected
	NSArray *selectedObjects = [self selectedObjects];
	BOOL addToSelection = NO;
	if ([selectedObjects containsObject:old])
	{
		addToSelection = YES;
	}
	[self removeObject:old];
	[self addObject:new];
	[self rearrangeObjects];
	if (addToSelection)
		[self addSelectedObjects:[NSArray arrayWithObject:new]];
//	[[self content] replaceObject:old withObject:new];
}

- (void)removeAll
{
	[self removeObjects:[self content]];
}

@end

// add (void)replaceObject:(id)old withObject:(id)new to NSMutableArray
@interface NSMutableArray (replaceObject)
- (void)replaceObject:(id)old withObject:(id)new;
@end

// adds to NSMutableArray
@implementation NSMutableArray (replaceObject)

/* replaces the first instance of old with new
*/
- (void)replaceObject:(id)old withObject:(id)new
{
	int index = [self indexOfObject:old];
	if (index == NSNotFound)
		NSLog(@"replaceObject:withObject: object to replace not found!");
	else
		[self replaceObjectAtIndex:index withObject:new];
}

@end
