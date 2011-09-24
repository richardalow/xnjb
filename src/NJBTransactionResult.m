//
//  NJBTransactionResult.m
//  XNJB
//
//  Created by Richard Low on Mon Jul 26 2004.
//

/* The object passed around to indicate
 * success or failure of some communication with
 * the NJB. Really is just a BOOL.
 */

#import "NJBTransactionResult.h"

@implementation NJBTransactionResult

// init/dealloc methods

- (id)initWithSuccess:(BOOL)newSuccess
{
	return [self initWithSuccess:newSuccess resultString:@"" extendedErrorString:@""];
}

- (id)initWithSuccess:(BOOL)newSuccess resultString:(NSString *)newResultString
{
	return [self initWithSuccess:newSuccess resultString:newResultString extendedErrorString:@""];
}

- (id)initWithSuccess:(BOOL)newSuccess resultString:(NSString *)newResultString extendedErrorString:(NSString *)newExtendedErrorString
{
  if (self = [super init])
  {
    [self setSuccess:newSuccess];
		[self setResultString:newResultString];
    [self setExtendedErrorString:newExtendedErrorString];
  }
  return self;
}

- (void)dealloc
{
	[resultString release];
  [extendedErrorString release];
	[super dealloc];
}

// accessor methods

- (void)setSuccess:(BOOL)newSuccess
{
	success = newSuccess;
}

- (BOOL)success
{
	return success;
}

- (void)setResultString:(NSString *)newResultString
{
	if (newResultString == nil)
		newResultString = @"";
	// get immutable copy
	newResultString = [NSString stringWithString:newResultString];
	[newResultString retain];
	[resultString release];
	resultString = newResultString;
}

- (NSString *)resultString
{
	return resultString;
}

- (void)setExtendedErrorString:(NSString *)newExtendedErrorString
{
  if (newExtendedErrorString == nil)
		newExtendedErrorString = @"";
	// get immutable copy
	newExtendedErrorString = [NSString stringWithString:newExtendedErrorString];
	[newExtendedErrorString retain];
	[extendedErrorString release];
	extendedErrorString = newExtendedErrorString;
}

- (NSString *)extendedErrorString
{
  return extendedErrorString;
}

@end
