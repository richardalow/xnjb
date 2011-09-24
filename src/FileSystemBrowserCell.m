//
//  FSBrowserCell.m
//
//  FSBrowserCell knows how to display file system info obtained from an FSNodeInfo object.

#import "FileSystemBrowserNode.h"
#import "FileSystemBrowserCell.h"

#define ICON_INSET_VERT		2.0	/* The size of empty space between the icon end the top/bottom of the cell */ 
#define ICON_SIZE 		16.0	/* Our Icons are ICON_SIZE x ICON_SIZE */
#define ICON_INSET_HORIZ	4.0	/* Distance to inset the icon from the left edge. */
#define ICON_TEXT_SPACING	2.0	/* Distance between the end of the icon and the text part */

@interface FileSystemBrowserCell (PrivateUtilities)
// This is a category in FSBrowserCell since it somewhat UI related and its a good idea to keep the UI part separate from the low level parts.
+ (NSDictionary*)stringAttributesForNode:(FileSystemBrowserNode *)node;
@end

@implementation FileSystemBrowserCell

- (void)dealloc {
    [iconImage release];
    iconImage = nil;
    [super dealloc];
}

- (id)init
{
	if (self = [super init])
	{
		iconImage = nil;
	}
	return self;
}

- (void)setAttributedStringValueFromFileSystemBrowserNode:(FileSystemBrowserNode*)node {
	// Given a particular FSNodeInfo object set up our display properties.
	NSString *stringValue = [node lastPathComponent];
	
	// Set the text part.   FileSystemBrowserNode will format the string (underline, bold, etc...) based on various properties of the file.
	[self setAttributedStringValue: [[[NSAttributedString alloc] initWithString:stringValue attributes:[FileSystemBrowserCell stringAttributesForNode:node]] autorelease]];

	// Set the image part.  FileSystemBrowserNode knows how to look up the proper icon to use for a given file/directory.
	[self setIconImage: [node iconImageOfSize:NSMakeSize(ICON_SIZE,ICON_SIZE)]];
	
	// If we don't have access to the file, make sure the user can't select it!
	// May already have been disabled so respect that
	if ([self isEnabled])
		[self setEnabled: [node isReadable]];

	// Make sure the cell knows if it has children or not.
	[self setLeaf:![node isDirectory]];
}

- (void)setIconImage: (NSImage *)image {
  if (image != iconImage)
  {
    [iconImage release];
    iconImage = image;
    [iconImage retain];
  }
    
    // Make sure the image is going to display at the size we want.
    [iconImage setSize: NSMakeSize(ICON_SIZE,ICON_SIZE)];
}

- (NSImage*)iconImage {
    return iconImage;
}

- (NSSize)cellSizeForBounds:(NSRect)aRect {
    // Make our cells a bit higher than normal to give some additional space for the icon to fit.
    NSSize theSize = [super cellSizeForBounds:aRect];
    theSize.width += [[self iconImage] size].width + ICON_INSET_HORIZ + ICON_INSET_HORIZ;
    theSize.height = ICON_SIZE + ICON_INSET_VERT * 2.0;
    return theSize;
}

- (void)drawInteriorWithFrame:(NSRect)cellFrame inView:(NSView *)controlView {    
	if (iconImage != nil) {
		NSSize imageSize = [iconImage size];
		NSRect imageFrame, highlightRect, textFrame;
	
		// Divide the cell into 2 parts, the image part (on the left) and the text part.
		NSDivideRect(cellFrame, &imageFrame, &textFrame, ICON_INSET_HORIZ + ICON_TEXT_SPACING + imageSize.width, NSMinXEdge);
    imageFrame.origin.x += ICON_INSET_HORIZ;
    imageFrame.size = imageSize;

		// Adjust the image frame top account for the fact that we may or may not be in a flipped control view, since when compositing
		// the online documentation states: "The image will have the orientation of the base coordinate system, regardless of the destination coordinates".
    if ([controlView isFlipped])
				imageFrame.origin.y += ceil((textFrame.size.height + imageFrame.size.height) / 2);
    else
				imageFrame.origin.y += ceil((textFrame.size.height - imageFrame.size.height) / 2);

		// Depending on the current state, set the color we will highlight with.
    if ([self isHighlighted])
		{
			// use highlightColorInView instead of [NSColor selectedControlColor] since NSBrowserCell slightly dims all cells except those in the right most column.
	    // The return value from highlightColorInView will return the appropriate one for you. 
	    [[self highlightColorInView: controlView] set];
		}
		else
		{
	    [[NSColor controlBackgroundColor] set];
		}

		// Draw the highligh, bu only the portion that won't be caught by the call to [super drawInteriorWithFrame:...] below.  No need to draw parts 2 times!
		highlightRect = NSMakeRect(NSMinX(cellFrame), NSMinY(cellFrame), NSWidth(cellFrame) - NSWidth(textFrame), NSHeight(cellFrame));
		NSRectFill(highlightRect);
	
		// Blit the image.
    [iconImage compositeToPoint:imageFrame.origin operation:NSCompositeSourceOver];
    
		// Have NSBrowser kindly draw the text part, since it knows how to do that for us, no need to re-invent what it knows how to do.
		[super drawInteriorWithFrame:textFrame inView:controlView];
	} else {
	// Atleast draw something if we couldn't find an icon.  You may want to do something more intelligent.
   	[super drawInteriorWithFrame:cellFrame inView:controlView];
  }
}

// disable the toopltip support - it crashes on Leopard because we don't link against the new library.  Not sure why though...
- (NSRect)expansionFrameWithFrame:(NSRect)cellFrame inView:(NSView *)view {
  NSRect r;
  memset(&r, 0, sizeof(r));
  return r;
}

@end

@implementation FileSystemBrowserCell (PrivateUtilities)

+ (NSDictionary*)stringAttributesForNode:(FileSystemBrowserNode*)node {
    NSMutableDictionary *attrs = [NSMutableDictionary dictionary];
    
    [attrs setObject:[NSFont systemFontOfSize:[NSFont systemFontSize]] forKey:NSFontAttributeName];
		
		NSMutableParagraphStyle *style = [[NSMutableParagraphStyle alloc] init];
		[style setLineBreakMode:NSLineBreakByTruncatingMiddle];
		[attrs setObject:style forKey:NSParagraphStyleAttributeName];
		[style release];

    return attrs;
}

@end

