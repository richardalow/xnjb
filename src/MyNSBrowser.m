//
//  MyNSBrowser.m
//  XNJB
//
//  Created by Richard Low on 01/07/2005.
//

#import "MyNSBrowser.h"

// declare the private methods
@interface MyNSBrowser (PrivateAPI)
- (NSArray *)pathComponents:(NSString *)path;
@end

@implementation MyNSBrowser

/* return the path components for the column
 * column (see pathComponents:).  No extra objects
 * at beginning or end for extra path delimiters.
 */
- (NSArray *)pathComponentsToColumn:(int)column
{
	return [self pathComponents:[super pathToColumn:column]];
}

/* return the path components for the current
 * path (see pathComponents:).  No extra objects
 * at beginning or end for extra path delimiters.
 */
- (NSArray *)pathComponents
{
	return [self pathComponents:[self path]];
}

/* this returns an NSArray of path components
 * for the path string path. This will not have
 * any blanks or path delimeters at the beginning.
 * Will have extra object at the end if path ends
 * in a path delimiter.
 */
- (NSArray *)pathComponents:(NSString *)path
{
	NSArray *components = [path pathComponents];
	if ([components count] == 0)
		return components;
	NSMutableArray *mutableComponents = [NSMutableArray arrayWithArray:components];
	[mutableComponents removeObjectAtIndex:0];
	return [NSArray arrayWithArray:mutableComponents];
}

/* returns the path string of the right most selected directory
 * i.e. the current selected directory or containing directory
 * of current selected file. Uses path so is the last selected item
 * will always begin and end in a '/'.
 */
- (NSString *)containingPath
{
	NSString *path = [self path];
	if ([path length] == 0)
		path = @"";
	if ([[self selectedCell] isLeaf])
	{
		if ([self selectedColumn] == 0)
			path = @"";
		else
			// stringByDeletingLastPathComponent loses the / from the end
			path = [path stringByDeletingLastPathComponent];
	}
	path = [NSString stringWithFormat:@"%@/", path];
	return path;
}

/* returns the components for containingPath.
 * Will have no extra objects at beginning/end.
 */
- (NSArray *)containingPathComponents
{
	NSArray *pathComponents = [self pathComponents];
	if ([[self selectedCell] isLeaf])
	{
		NSMutableArray *pathComponentsMutable = [NSMutableArray arrayWithArray:pathComponents];
		[pathComponentsMutable removeLastObject];
		pathComponents = [NSArray arrayWithArray:pathComponentsMutable];
	}
	return pathComponents;
}

@end
