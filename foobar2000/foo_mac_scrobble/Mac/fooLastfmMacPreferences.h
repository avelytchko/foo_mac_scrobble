//
//  fooLastfmMacPreferences.h
//  foo_mac_scrobble
//
//  Created by Oleksandr Velychko on 09/11/2025.
//

#import <Cocoa/Cocoa.h>

@interface fooLastfmMacPreferences : NSViewController

@property(nonatomic, strong) NSTextField* apiKeyField;
@property(nonatomic, strong) NSTextField* apiSecretField;
@property(nonatomic, strong) NSSlider* thresholdSlider;
@property(nonatomic, strong) NSTextField* thresholdLabel;
@property(nonatomic, strong) NSButton* enabledCheckbox;
@property(nonatomic, strong) NSButton* debugCheckbox; // NEW: Debug logging checkbox
@property(nonatomic, strong) NSTextField* statusLabel;
@property(nonatomic, strong) NSButton* authButton;

@end
