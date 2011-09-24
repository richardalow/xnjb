//
//  FSBrowserCell.h
//
//  FSBrowserCell knows how to display file system info obtained from an FSNodeInfo object.

#import <Cocoa/Cocoa.h>

@interface FileSystemBrowserCell : NSBrowserCell { 
@private
    NSImage *iconImage;
}

- (void)setAttributedStringValueFromFileSystemBrowserNode:(FileSystemBrowserNode *)node;
- (void)setIconImage: (NSImage *)image;
- (NSImage*)iconImage;

@end

