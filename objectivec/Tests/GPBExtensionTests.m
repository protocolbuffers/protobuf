#import "GPBTestUtilities.h"
#import "GPBUtilities.h"
#import "GPBUtilities_PackagePrivate.h"
#import "objectivec/Tests/Unittest.pbobjc.h"
#import "objectivec/Tests/UnittestExtensions.pbobjc.h"

@interface GPBExtensionTests : XCTestCase
@end

@implementation GPBExtensionTests

- (void)testExtensionLeak {
  Car *car = [[Car alloc] init];
  car.make = @"Honda";
  car.model = @"Civic";
  car.year = @"2025";
  [car setExtension:[UnittestExtensionsRoot notes] value:@"This is a test car"];
  [car setExtension:[DMV vin] value:@"VIN"];
  NSLog(@"car: %@", car);
}

@end
