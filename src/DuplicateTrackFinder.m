//
//  DuplicateTrackFinder.m
//  XNJB
//
//  Created by Richard Low on 11/01/2005.
//  Copyright 2005 __MyCompanyName__. All rights reserved.
//

#import "DuplicateTrackFinder.h"
#import "Track.h"

@interface DuplicateTrackFinder (PrivateAPI)
+ (unsigned)levenshteinDistanceFrom:(NSString *)s to:(NSString *)t;
+ (unsigned)mina:(unsigned)a b:(unsigned)b c:(unsigned)c;
@end

@implementation DuplicateTrackFinder

+ (unsigned)levenshteinDistanceFrom:(NSString *)s to:(NSString *)t
{
	unsigned i, j;
	unsigned cost;
	unsigned n = [s length];
	unsigned m = [t length];
	unsigned **d = malloc((n+1) * sizeof(unsigned*));
	for (i = 0; i <= n; i++)
	{
		d[i] = malloc((m+1) * sizeof(unsigned));
	}
	
	for (i = 0; i <=n; i++)
	{
		d[i][0] = i;
		for (j = 0; j <= m; j++)
		{
			d[0][j] = j;
		}
	}
	
	for (i = 1; i <= n; i++)
	{
		for (j = 1; j <= m; j++)
		{
			if ([s characterAtIndex:i-1] == [t characterAtIndex:j-1])
				cost = 0;
			else
				cost = 1;
			d[i][j] = [DuplicateTrackFinder mina:(d[i-1][j  ] + 1)      // insertion
																				 b:(d[i  ][j-1] + 1)		  // deletion
																				 c:(d[i-1][j-1] + cost)]; // substitution
		}
	}
	unsigned distance = d[n][m];
	for (i = 0; i <= n; i++)
		free(d[i]);
	free(d);
	
	return distance;
}

+ (unsigned)mina:(unsigned)a b:(unsigned)b c:(unsigned)c
{
	if (a < b)
	{
		if (a < c)
			return a;
	  else
			return c;
	}
	else
	{
		if (b < c)
			return b;
		else
			return c;
	}
}

/* This function will return an array of potentially duplicate tracks using the
 * tolerances given.
 * Set carryOn to NO to get the loop to stop.
 * The algorithm works on the basis that there will not be many duplicates.
 * So the tracks are sorted by length, then we go up the list to find tracks with
 * lengths within the tolerance, and stop when we get too far.  We only do 
 * expensive Levenshtein distance calculations on the strings when all other properties
 * match within the tolerances.
 */
+ (NSMutableArray *)findDuplicates:(NSMutableArray *)tracks
												 lengthTol:(unsigned)lengthTol
											 filesizeTol:(unsigned)filesizeTol
													titleTol:(unsigned)titleTol
												 artistTol:(unsigned)artistTol
													 carryOn:(MyBool *)carryOn
{
	// time it
	NSDate *date = [NSDate date];
	
	// todo: if 3 duplicates will likely get 2 copies of the same track
	unsigned i, j;
	BOOL duplicateFoundForThisi = NO;
	NSMutableArray *duplicates = [[NSMutableArray alloc] init];
	unsigned count = [tracks count];
	
	[tracks sortUsingSelector:@selector(compareByLength:)];
	
	BOOL bLengthTooFarDifferent = NO;
	
	for (i = 0; i < count; i++)
	{
		Track *a = (Track *)[tracks objectAtIndex:i];
		duplicateFoundForThisi = NO;
		bLengthTooFarDifferent = NO;
		for (j = i+1; j < count && !bLengthTooFarDifferent; j++)
		{
			if (![carryOn value])
			{
				return nil;
			}
			Track *b = (Track *)[tracks objectAtIndex:j];
			//NSLog(@"comparing track %@ with %@", [a title], [b title]);
			if (abs([a length] - [b length]) <= lengthTol)
			{
				if (abs([a filesize] - [b filesize]) <= filesizeTol
				 && abs([[a title] length] - [[b title] length]) <= titleTol
				 && abs([[a artist] length] - [[b artist] length]) <= artistTol
				 && [DuplicateTrackFinder levenshteinDistanceFrom:[a title] to:[b title]] <= titleTol
				 && [DuplicateTrackFinder levenshteinDistanceFrom:[a artist] to:[b artist]] <= artistTol)
				{
					//NSLog(@"duplicate found!");
					duplicateFoundForThisi = YES;
					[duplicates addObject:b];
				}
			}
			else
				bLengthTooFarDifferent = YES;
		}
		if (duplicateFoundForThisi)
			[duplicates addObject:a];
	}
	
	double time = -[date timeIntervalSinceNow];
	NSLog(@"Time taken: %f", time);
	
	NSLog(@"Duplicates found: %d", [duplicates count]);
	
	return [duplicates autorelease];
}

@end
