//
//  MyBool.m
//  XNJB
//
//  Created by Richard Low on 01/04/2005.
//  Copyright 2005 __MyCompanyName__. All rights reserved.
//

#import "MyBool.h"


@implementation MyBool

- (id)init
{
	return [self initWithValue:NO];
}

- (id)initWithValue:(BOOL) newValue
{
	if (self = [super init])
	{
		[self setValue:newValue];
	}
	return self;
}

- (void)setValue:(BOOL) newValue
{
	value = newValue;
}

- (BOOL)value
{
	return value;
}

@end
