#import <UIKit/UIKit.h>

@interface AppDelegate : UIResponder <UIApplicationDelegate>
@property (strong, nonatomic) UIWindow *window;
@end

@implementation AppDelegate

@synthesize window;

- (BOOL)application:(UIApplication *)application
    didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
  #pragma unused (application, launchOptions)

  self.window = [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
  self.window.backgroundColor = [UIColor whiteColor];
  [self.window makeKeyAndVisible];
  self.window.rootViewController = [[UIViewController alloc] init];

  UILabel *label =
      [[UILabel alloc] initWithFrame:CGRectMake(0, 200, CGRectGetWidth(self.window.frame), 40)];
  label.text = @"Protocol Buffer Test Harness";
  label.textAlignment = NSTextAlignmentCenter;
  [self.window addSubview:label];

  return YES;
}

@end

int main(int argc, char * argv[]) {
  @autoreleasepool {
    return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
  }
}
