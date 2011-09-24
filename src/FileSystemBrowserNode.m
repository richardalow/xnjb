//
//  FSNodeInfo.m
//
//  FSNodeInfo encapsulates information about a file or directory.
//  This implementation is not necessarily the best way to do something like this,
//  it is simply a wrapper to make the rest of the browser code easy to follow.

#import "FileSystemBrowserNode.h"

@implementation FileSystemBrowserNode 

+ (FileSystemBrowserNode *)nodeWithParent:(FileSystemBrowserNode *)parent atRelativePath:(NSString *)path {
    return [[[FileSystemBrowserNode alloc] initWithParent:parent atRelativePath:path] autorelease];
}

- (id)initWithParent:(FileSystemBrowserNode *)parent atRelativePath:(NSString*)path {    
    self = [super init];
    if (self==nil) return nil;
    
    parentNode = parent;
    relativePath = [path retain];
    
    return self;
}

- (void)dealloc {
    // parentNode is not released since we never retained it.
    [relativePath release];
    relativePath = nil;
    parentNode = nil;
    [super dealloc];
}

- (int)rowIndexForNode:(NSString *)nodeName
{
	NSArray *subNodes = [self subNodes];
	
	FileSystemBrowserNode *node;
	NSEnumerator *enumerator = [subNodes objectEnumerator];
	
	int index = 0;
	BOOL found = NO;
	
	while (!found && (node = [enumerator nextObject]) != nil)
	{
    if ([node isVisible])
		{
			if ([[node lastPathComponent] isEqualToString:nodeName])
			{
				found = YES;
			}
			else
			{
				index++;
			}
		}
	}
	
	if (found)
		return index;
	else
		return -1;
}

- (NSArray *)subNodes {
    NSString       *subNodePath = nil;
		NSArray				 *subNodePathsArray = [[NSFileManager defaultManager] directoryContentsAtPath: [self absolutePath]];
		
		// sort them
		subNodePathsArray = [subNodePathsArray sortedArrayUsingSelector:@selector(caseInsensitiveCompare:)];
		
    NSEnumerator   *subNodePaths = [subNodePathsArray objectEnumerator];
    NSMutableArray *subNodes = [NSMutableArray array];
    
    while ((subNodePath=[subNodePaths nextObject])) {
        FileSystemBrowserNode *node = [FileSystemBrowserNode nodeWithParent:self atRelativePath: subNodePath];
        [subNodes addObject: node];
    }
    return subNodes;
}

- (NSArray *)visibleSubNodes {
    FileSystemBrowserNode *subNode = nil;
    NSEnumerator *allSubNodes = [[self subNodes] objectEnumerator];
    NSMutableArray *visibleSubNodes = [NSMutableArray array];
    
    while ((subNode=[allSubNodes nextObject])) {
        if ([subNode isVisible]) [visibleSubNodes addObject: subNode];
    }
    return visibleSubNodes;
}

- (BOOL)isLink {
    NSDictionary *fileAttributes = [[NSFileManager defaultManager] fileAttributesAtPath:[self absolutePath] traverseLink:NO];
    return [[fileAttributes objectForKey:NSFileType] isEqualToString:NSFileTypeSymbolicLink];
}

- (BOOL)isDirectory {
    BOOL isDir = NO;
    BOOL exists = [[NSFileManager defaultManager] fileExistsAtPath:[self absolutePath] isDirectory:&isDir];
    return (exists && isDir);
}

- (BOOL)isReadable {
    return [[NSFileManager defaultManager] isReadableFileAtPath: [self absolutePath]];
}

- (BOOL)isVisible {
	NSString *lastPathComponent = [self lastPathComponent];
	if ([lastPathComponent length] == 0)
		return NO;
	else if ([lastPathComponent characterAtIndex:0] == '.')
		return NO;
	return YES;
}

- (NSString*)lastPathComponent {
    return [relativePath lastPathComponent];
}

- (NSString*)absolutePath {
    NSString *result = relativePath;
    if(parentNode!=nil) {
        NSString *parentAbsPath = [parentNode absolutePath];
        if ([parentAbsPath isEqualToString: @"/"]) parentAbsPath = @"";
        result = [NSString stringWithFormat: @"%@/%@", parentAbsPath, relativePath];
    }
    return result;
}

- (NSImage*)iconImageOfSize:(NSSize)size {
    NSString *path = [self absolutePath];
    NSImage *nodeImage = nil;
    
    nodeImage = [[NSWorkspace sharedWorkspace] iconForFile:path];
		
    if (!nodeImage) {
        // No icon for actual file, try the extension.
        nodeImage = [[NSWorkspace sharedWorkspace] iconForFileType:[path pathExtension]];
    }
    [nodeImage setSize: size];
    
    return nodeImage;
}

@end
