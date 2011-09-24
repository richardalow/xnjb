//
//  NJBTransactionResult.h
//  XNJB
//
//  Created by Richard Low on Mon Jul 26 2004.
//

#import <Foundation/Foundation.h>


@interface NJBTransactionResult : NSObject {
	BOOL success;
	NSString *resultString;
  NSString *extendedErrorString;
}
- (id)initWithSuccess:(BOOL)newSuccess;
- (id)initWithSuccess:(BOOL)newSuccess resultString:(NSString *)newResultString;
- (id)initWithSuccess:(BOOL)newSuccess resultString:(NSString *)newResultString extendedErrorString:(NSString *)newExtendedErrorString;
- (void)setSuccess:(BOOL)newSuccess;
- (BOOL)success;
- (void)setResultString:(NSString *)newResultString;
- (NSString *)resultString;
- (void)setExtendedErrorString:(NSString *)newExtendedErrorString;
- (NSString *)extendedErrorString;


@end
