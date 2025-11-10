//
//  fooLastfmMacPreferences.mm
//  foo_mac_scrobble
//
//  Created by Oleksandr Velychko on 09/11/2025.
//

#import "fooLastfmMacPreferences.h"
#include "../stdafx.h"
#include "../config.h"
#include "../lastfm_api.h"
#include "../safe_log_utils.h"

// Masking helper to obscure sensitive data (API key/secret)
static std::string mask_show_first_last(const std::string &s, size_t first = 2, size_t last = 2) {
    if (s.empty()) return "";
    if (s.size() <= first + last) return std::string(s.size(), '*');
    std::string masked = s.substr(0, first);
    masked += std::string(s.size() - first - last, '*');
    masked += s.substr(s.size() - last);
    return masked;
}

@interface fooLastfmMacPreferences ()
{
    BOOL showingSecrets;
    std::string realApiKey;
    std::string realApiSecret;
    NSButton *revealButton; // Show/Hide button for API key/secret
}
@end

@implementation fooLastfmMacPreferences

- (void)viewDidLoad {
    [super viewDidLoad];
    [self setupUI];
    showingSecrets = NO;
}

- (void)viewWillAppear {
    [super viewWillAppear];

    // Load current settings from configuration
    realApiKey    = foo_lastfm::cfg_api_key.get();
    realApiSecret = foo_lastfm::cfg_api_secret.get();

    // Display masked API key and secret
    std::string maskedKey    = mask_show_first_last(realApiKey, 2, 2);
    std::string maskedSecret = mask_show_first_last(realApiSecret, 2, 2);
    [self.apiKeyField setStringValue:@(maskedKey.c_str())];
    [self.apiSecretField setStringValue:@(maskedSecret.c_str())];

    // Initialize Show/Hide button
    if (revealButton) {
        [revealButton setTitle:@"Show"];
    }
    showingSecrets = NO;

    [self.thresholdSlider setIntegerValue:foo_lastfm::cfg_scrobble_percent.get()];
    [self.enabledCheckbox setState:foo_lastfm::cfg_enabled.get() ? NSControlStateValueOn : NSControlStateValueOff];
    [self.debugCheckbox setState:foo_lastfm::cfg_debug_enabled.get() ? NSControlStateValueOn : NSControlStateValueOff];
    [self updateThresholdLabel];
    [self updateStatusLabel];
}

- (void)setupUI {
    // Create main stack view for organizing UI elements
    NSStackView* stackView = [[NSStackView alloc] init];
    [stackView setOrientation:NSUserInterfaceLayoutOrientationVertical];
    [stackView setAlignment:NSLayoutAttributeLeading];
    [stackView setSpacing:8];
    [stackView setTranslatesAutoresizingMaskIntoConstraints:NO];
    [self.view addSubview:stackView];

    // Set standard system margins
    CGFloat inset = 19.0; // Adjustable margin size
    [stackView.leadingAnchor constraintEqualToAnchor:self.view.leadingAnchor constant:inset].active = YES;
    [stackView.trailingAnchor constraintEqualToAnchor:self.view.trailingAnchor constant:-inset].active = YES;
    [stackView.topAnchor constraintEqualToAnchor:self.view.topAnchor constant:inset].active = YES;
    [stackView.bottomAnchor constraintEqualToAnchor:self.view.bottomAnchor constant:-inset].active = YES;
    
    // Add title label
    NSTextField* titleLabel = [[NSTextField alloc] init];
    [titleLabel setStringValue:@"Last.fm Scrobbler Settings"];
    [titleLabel setBezeled:NO];
    [titleLabel setDrawsBackground:NO];
    [titleLabel setEditable:NO];
    [titleLabel setFont:[NSFont systemFontOfSize:15]];
    [stackView addArrangedSubview:titleLabel];

    // Add spacer
    NSView* spacer1 = [[NSView alloc] init];
    [spacer1.heightAnchor constraintEqualToConstant:8].active = YES;
    [stackView addArrangedSubview:spacer1];

    // Add API Key label
    NSTextField* apiKeyLabel = [[NSTextField alloc] init];
    [apiKeyLabel setStringValue:@"API Key:"];
    [apiKeyLabel setBezeled:NO];
    [apiKeyLabel setDrawsBackground:NO];
    [apiKeyLabel setEditable:NO];
    [stackView addArrangedSubview:apiKeyLabel];

    // Add API Key input field
    self.apiKeyField = [[NSTextField alloc] init];
    [self.apiKeyField setPlaceholderString:@"Enter your Last.fm API Key"];
    [self.apiKeyField setTarget:self];
    [self.apiKeyField setAction:@selector(onApiKeyChanged:)];
    [stackView addArrangedSubview:self.apiKeyField];

    // Add API Secret label
    NSTextField* apiSecretLabel = [[NSTextField alloc] init];
    [apiSecretLabel setStringValue:@"API Secret:"];
    [apiSecretLabel setBezeled:NO];
    [apiSecretLabel setDrawsBackground:NO];
    [apiSecretLabel setEditable:NO];
    [stackView addArrangedSubview:apiSecretLabel];

    // Add API Secret input field
    self.apiSecretField = [[NSTextField alloc] init];
    [self.apiSecretField setPlaceholderString:@"Enter your Last.fm API Secret"];
    [self.apiSecretField setTarget:self];
    [self.apiSecretField setAction:@selector(onApiSecretChanged:)];
    [stackView addArrangedSubview:self.apiSecretField];
    
    // Add Show/Hide button for toggling API key/secret visibility
    revealButton = [[NSButton alloc] init];
    [revealButton setBezelStyle:NSBezelStyleRounded];
    [revealButton setTitle:@"Show"];
    [revealButton setTarget:self];
    [revealButton setAction:@selector(onToggleShow:)];
    [revealButton.widthAnchor constraintEqualToConstant:80].active = YES;
    [stackView addArrangedSubview:revealButton];

    // Add spacer
    NSView* spacer2 = [[NSView alloc] init];
    [spacer2.heightAnchor constraintEqualToConstant:8].active = YES;
    [stackView addArrangedSubview:spacer2];

    // Add scrobble threshold label
    NSTextField* thresholdMainLabel = [[NSTextField alloc] init];
    [thresholdMainLabel setStringValue:@"Scrobble after listening to:"];
    [thresholdMainLabel setBezeled:NO];
    [thresholdMainLabel setDrawsBackground:NO];
    [thresholdMainLabel setEditable:NO];
    [stackView addArrangedSubview:thresholdMainLabel];

    // Add threshold slider container
    NSView* sliderContainer = [[NSView alloc] init];
    self.thresholdSlider = [[NSSlider alloc] init];
    [self.thresholdSlider setMinValue:30];
    [self.thresholdSlider setMaxValue:100];
    [self.thresholdSlider setTarget:self];
    [self.thresholdSlider setAction:@selector(onThresholdChanged:)];
    [self.thresholdSlider setTranslatesAutoresizingMaskIntoConstraints:NO];
    [sliderContainer addSubview:self.thresholdSlider];

    self.thresholdLabel = [[NSTextField alloc] init];
    [self.thresholdLabel setStringValue:@"50%"];
    [self.thresholdLabel setBezeled:NO];
    [self.thresholdLabel setDrawsBackground:NO];
    [self.thresholdLabel setEditable:NO];
    [self.thresholdLabel setAlignment:NSTextAlignmentRight];
    [self.thresholdLabel.widthAnchor constraintEqualToConstant:50].active = YES;
    [self.thresholdLabel setTranslatesAutoresizingMaskIntoConstraints:NO];
    [sliderContainer addSubview:self.thresholdLabel];

    [self.thresholdSlider.leadingAnchor constraintEqualToAnchor:sliderContainer.leadingAnchor].active = YES;
    [self.thresholdSlider.trailingAnchor constraintEqualToAnchor:self.thresholdLabel.leadingAnchor constant:-8].active = YES;
    [self.thresholdSlider.centerYAnchor constraintEqualToAnchor:sliderContainer.centerYAnchor].active = YES;
    [self.thresholdLabel.trailingAnchor constraintEqualToAnchor:sliderContainer.trailingAnchor].active = YES;
    [self.thresholdLabel.centerYAnchor constraintEqualToAnchor:sliderContainer.centerYAnchor].active = YES;
    [sliderContainer.heightAnchor constraintEqualToConstant:24].active = YES;
    [stackView addArrangedSubview:sliderContainer];

    // Add spacer
    NSView* spacer3 = [[NSView alloc] init];
    [spacer3.heightAnchor constraintEqualToConstant:8].active = YES;
    [stackView addArrangedSubview:spacer3];

    // Add enable scrobbling checkbox
    self.enabledCheckbox = [[NSButton alloc] init];
    [self.enabledCheckbox setButtonType:NSButtonTypeSwitch];
    [self.enabledCheckbox setTitle:@"Enable Scrobbling"];
    [self.enabledCheckbox setTarget:self];
    [self.enabledCheckbox setAction:@selector(onEnabledChanged:)];
    [stackView addArrangedSubview:self.enabledCheckbox];

    // Add debug logging checkbox
    self.debugCheckbox = [[NSButton alloc] init];
    [self.debugCheckbox setButtonType:NSButtonTypeSwitch];
    [self.debugCheckbox setTitle:@"Enable Debug Logging"];
    [self.debugCheckbox setTarget:self];
    [self.debugCheckbox setAction:@selector(onDebugChanged:)];
    [stackView addArrangedSubview:self.debugCheckbox];

    // Add spacer
    NSView* spacer4 = [[NSView alloc] init];
    [spacer4.heightAnchor constraintEqualToConstant:16].active = YES;
    [stackView addArrangedSubview:spacer4];

    // Add status label
    self.statusLabel = [[NSTextField alloc] init];
    [self.statusLabel setStringValue:@"Not authenticated"];
    [self.statusLabel setBezeled:NO];
    [self.statusLabel setDrawsBackground:NO];
    [self.statusLabel setEditable:NO];
    [self.statusLabel setAlignment:NSTextAlignmentCenter];
    [stackView addArrangedSubview:self.statusLabel];

    // Add authentication button
    self.authButton = [[NSButton alloc] init];
    [self.authButton setTitle:@"Authenticate"];
    [self.authButton setBezelStyle:NSBezelStyleRounded];
    [self.authButton setTarget:self];
    [self.authButton setAction:@selector(onAuthenticateClicked:)];
    [self.authButton.widthAnchor constraintEqualToConstant:150].active = YES;
    [self.authButton setAlignment:NSCenterTextAlignment];
    [stackView addArrangedSubview:self.authButton];

    // Update status label after UI setup
    [self updateStatusLabel];
}

- (void)updateStatusLabel {
    if (!self.statusLabel) return;

    // Update UI based on authentication status
    if (foo_lastfm::g_lastfm_api && foo_lastfm::g_lastfm_api->has_saved_session()) {
        NSString* username = [NSString stringWithUTF8String:foo_lastfm::cfg_username.get()];
        if (username.length > 0) {
            [self.statusLabel setStringValue:[NSString stringWithFormat:@"✓ Authenticated as: %@", username]];
        } else {
            [self.statusLabel setStringValue:@"✓ Authenticated (session saved)"];
        }
        [self.statusLabel setTextColor:[NSColor systemGreenColor]];
        [self.authButton setTitle:@"Re-authenticate"];
    } else {
        [self.statusLabel setStringValue:@"Not authenticated"];
        [self.statusLabel setTextColor:[NSColor secondaryLabelColor]];
        [self.authButton setTitle:@"Authenticate"];
    }
}

- (IBAction)onToggleShow:(id)sender {
    showingSecrets = !showingSecrets;

    // Toggle visibility of API key and secret
    if (showingSecrets) {
        self.apiKeyField.stringValue    = @(realApiKey.c_str());
        self.apiSecretField.stringValue = @(realApiSecret.c_str());
        [revealButton setTitle:@"Hide"];
    } else {
        std::string maskedKey    = mask_show_first_last(realApiKey, 2, 2);
        std::string maskedSecret = mask_show_first_last(realApiSecret, 2, 2);
        self.apiKeyField.stringValue    = @(maskedKey.c_str());
        self.apiSecretField.stringValue = @(maskedSecret.c_str());
        [revealButton setTitle:@"Show"];
    }
}

- (void)updateThresholdLabel {
    [self.thresholdLabel setStringValue:[NSString stringWithFormat:@"%ld%%", (long)[self.thresholdSlider integerValue]]];
}

- (IBAction)onApiKeyChanged:(id)sender {
    std::string entered = [[self.apiKeyField stringValue] UTF8String];

    // Ignore input if in masked mode
    if (!showingSecrets && entered.find('*') != std::string::npos) {
        return;
    }

    // Update configuration and API with new API key
    realApiKey = entered;
    foo_lastfm::cfg_api_key.set(realApiKey.c_str());
    if (foo_lastfm::g_lastfm_api) {
        foo_lastfm::g_lastfm_api->set_credentials(realApiKey.c_str(), realApiSecret.c_str());
    }

    if (foo_lastfm::cfg_debug_enabled.get()) {
        FB2K_console_formatter()
            << "Last.fm: API Key updated (length: " << (int)realApiKey.length() << ")";
    }

    // Redraw masked key if in hide mode
    if (!showingSecrets) {
        std::string maskedKey = mask_show_first_last(realApiKey, 2, 2);
        self.apiKeyField.stringValue = @(maskedKey.c_str());
    }
}

- (IBAction)onApiSecretChanged:(id)sender {
    std::string entered = [[self.apiSecretField stringValue] UTF8String];

    // Ignore input if in masked mode
    if (!showingSecrets && (entered.empty() || entered.find('*') != std::string::npos)) {
        if (foo_lastfm::cfg_debug_enabled.get()) {
            FB2K_console_formatter() << "Last.fm: Secret change ignored (masked input)";
        }
        return;
    }

    // Update configuration and API with new API secret
    realApiSecret = entered;
    foo_lastfm::cfg_api_secret.set(realApiSecret.c_str());
    if (foo_lastfm::g_lastfm_api) {
        foo_lastfm::g_lastfm_api->set_credentials(realApiKey.c_str(), realApiSecret.c_str());
    }

    if (foo_lastfm::cfg_debug_enabled.get()) {
        FB2K_console_formatter()
            << "Last.fm: API Secret updated (length: " << (int)realApiSecret.length() << ")";
    }

    // Redraw masked secret if in hide mode
    if (!showingSecrets) {
        std::string maskedSecret = mask_show_first_last(realApiSecret, 2, 2);
        self.apiSecretField.stringValue = @(maskedSecret.c_str());
    }
}

- (IBAction)onThresholdChanged:(id)sender {
    // Update scrobble threshold in configuration
    foo_lastfm::cfg_scrobble_percent.set([self.thresholdSlider integerValue]);
    [self updateThresholdLabel];

    if (foo_lastfm::cfg_debug_enabled.get()) {
        FB2K_console_formatter()
            << "Last.fm: Scrobble threshold changed to "
            << (int)[self.thresholdSlider integerValue] << "%";
    }
}

- (IBAction)onEnabledChanged:(id)sender {
    // Enable or disable scrobbling
    bool enabled = [self.enabledCheckbox state] == NSControlStateValueOn;
    foo_lastfm::cfg_enabled.set(enabled);

    if (foo_lastfm::cfg_debug_enabled.get()) {
        FB2K_console_formatter()
            << "Last.fm: Scrobbling "
            << (enabled ? "ENABLED" : "DISABLED");
    }
}

- (IBAction)onDebugChanged:(id)sender {
    // Enable or disable debug logging
    bool debug_enabled = [self.debugCheckbox state] == NSControlStateValueOn;
    foo_lastfm::cfg_debug_enabled.set(debug_enabled);
    FB2K_console_formatter()
        << "Last.fm: Debug logging "
        << (debug_enabled ? "ENABLED" : "DISABLED");
}

- (IBAction)onAuthenticateClicked:(id)sender {
    if (!foo_lastfm::g_lastfm_api) {
        NSAlert* alert = [[NSAlert alloc] init];
        [alert setMessageText:@"API Error"];
        [alert setInformativeText:@"Last.fm API not initialized."];
        [alert setAlertStyle:NSAlertStyleCritical];
        [alert runModal];
        return;
    }

    // Retrieve API key and secret, using real values if fields are masked
    std::string keyStr, secretStr;
    {
        std::string fKey    = [[self.apiKeyField stringValue] UTF8String];
        std::string fSecret = [[self.apiSecretField stringValue] UTF8String];

        if (!showingSecrets && (fKey.find('*') != std::string::npos))    fKey = realApiKey;
        if (!showingSecrets && (fSecret.find('*') != std::string::npos)) fSecret = realApiSecret;

        keyStr = fKey;
        secretStr = fSecret;
    }

    // Validate credentials
    if (keyStr.empty() || secretStr.empty()) {
        NSAlert* alert = [[NSAlert alloc] init];
        [alert setMessageText:@"Missing Credentials"];
        [alert setInformativeText:@"Please enter both API Key and API Secret before authenticating."];
        [alert setAlertStyle:NSAlertStyleWarning];
        [alert runModal];
        return;
    }

    // Save credentials to configuration and API
    foo_lastfm::cfg_api_key.set(keyStr.c_str());
    foo_lastfm::cfg_api_secret.set(secretStr.c_str());
    realApiKey = keyStr;
    realApiSecret = secretStr;
    foo_lastfm::g_lastfm_api->set_credentials(keyStr.c_str(), secretStr.c_str());

    if (foo_lastfm::cfg_debug_enabled.get()) {
        FB2K_console_formatter() << "Last.fm: Credentials saved to config and API";
    }

    // Open authentication URL in browser
    std::string authURL = foo_lastfm::g_lastfm_api->get_auth_url();
    if (foo_lastfm::cfg_debug_enabled.get()) {
        std::string masked_url = authURL;
        const std::string key_param = "api_key=";
        auto pos = masked_url.find(key_param);
        if (pos != std::string::npos) {
            pos += key_param.size();
            auto end = masked_url.find_first_of("&?", pos);
            std::string api_key = masked_url.substr(pos, end - pos);
            masked_url.replace(pos, api_key.size(), redact_secret(api_key));
        }
        FB2K_console_formatter()
            << "Last.fm: Opening authentication URL: "
            << masked_url.c_str();
    }

    [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:[NSString stringWithUTF8String:authURL.c_str()]]];

    // Show authentication instructions
    NSAlert* alert = [[NSAlert alloc] init];
    [alert setMessageText:@"Last.fm Authentication"];
    [alert setInformativeText:@"📋 INSTRUCTIONS:\n\n"
        "1. Click 'Yes, allow access' in your browser\n\n"
        "2. The page will reload and show 'Invalid API key' error\n"
        "   ⚠️ This error is NORMAL and EXPECTED - ignore it!\n\n"
        "3. Copy the FULL URL from your browser's address bar\n"
        "   (Example: http://www.last.fm/api/auth?token=abc123...)\n\n"
        "4. Paste the URL below and click OK\n\n"
        "✅ The token in the URL is valid despite the error!"];
    [alert addButtonWithTitle:@"OK"];
    [alert addButtonWithTitle:@"Cancel"];

    NSTextField* tokenField = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 450, 24)];
    [tokenField setPlaceholderString:@"Paste full URL here (with ?token= in it)"];
    [alert setAccessoryView:tokenField];

    NSModalResponse response = [alert runModal];

    if (response == NSAlertFirstButtonReturn) {
        NSString* input = [tokenField stringValue];
        std::string inputStr = [input UTF8String];

        // Validate pasted URL
        NSURLComponents* comps = [NSURLComponents componentsWithString:input];
        BOOL validHost = NO;
        BOOL hasToken = NO;

        if (comps.host) {
            NSString* host = comps.host.lowercaseString;
            if ([host containsString:@"last.fm"]) validHost = YES;
        }

        if (comps.queryItems) {
            for (NSURLQueryItem* item in comps.queryItems) {
                if ([item.name isEqualToString:@"token"] && item.value.length > 0) {
                    hasToken = YES;
                    break;
                }
            }
        }

        if (!validHost) {
            NSAlert* warn = [[NSAlert alloc] init];
            [warn setMessageText:@"Unexpected URL Host"];
            [warn setInformativeText:@"The pasted URL is not from Last.fm.\nPlease make sure you copied the full Last.fm authorization URL."];
            [warn setAlertStyle:NSAlertStyleWarning];
            [warn runModal];

            if (foo_lastfm::cfg_debug_enabled.get()) {
                std::string input_str = [input UTF8String];
                const std::string token_param = "token=";
                auto pos = input_str.find(token_param);
                if (pos != std::string::npos) {
                    pos += token_param.size();
                    auto end = input_str.find_first_of("&?", pos);
                    std::string token_value = input_str.substr(pos, end - pos);
                    input_str.replace(pos, token_value.size(), redact_secret(token_value));
                }

                FB2K_console_formatter()
                    << "Last.fm WARNING: URL host invalid (" << input_str.c_str() << ")";
            }
            return;
        }

        if (!hasToken) {
            NSAlert* warn = [[NSAlert alloc] init];
            [warn setMessageText:@"Missing Token"];
            [warn setInformativeText:@"The pasted URL doesn't contain a ?token= parameter.\nMake sure you copied the entire address from the Last.fm browser page."];
            [warn setAlertStyle:NSAlertStyleWarning];
            [warn runModal];

            if (foo_lastfm::cfg_debug_enabled.get()) {
                FB2K_console_formatter()
                    << "Last.fm WARNING: URL missing token parameter";
            }

            return;
        }

        // DEBUG LOG
        if (foo_lastfm::cfg_debug_enabled.get()) {
            FB2K_console_formatter()
                << "Last.fm: Processing token input (length: "
                << (int)inputStr.length() << ")";
        }

        // Extract token from URL
        std::string token;
        size_t token_pos = inputStr.find("token=");
        if (token_pos != std::string::npos) {
            token = inputStr.substr(token_pos + 6);
            size_t bad_char = token.find_first_of("&#? \n\r\t");
            if (bad_char != std::string::npos) {
                token = token.substr(0, bad_char);
            }
        } else {
            token = inputStr;
            size_t bad_char = token.find_first_of("&#? \n\r\t");
            if (bad_char != std::string::npos) {
                token = token.substr(0, bad_char);
            }
        }

        if (token.empty()) {
            NSAlert* errorAlert = [[NSAlert alloc] init];
            [errorAlert setMessageText:@"No Token Found"];
            [errorAlert setInformativeText:@"Couldn't find a token in what you pasted.\n\nMake sure you copied the entire URL from your browser's address bar."];
            [errorAlert setAlertStyle:NSAlertStyleWarning];
            [errorAlert runModal];

            if (foo_lastfm::cfg_debug_enabled.get()) {
                FB2K_console_formatter() << "Last.fm ERROR: Failed to extract token from input";
            }
            return;
        }

        if (foo_lastfm::cfg_debug_enabled.get()) {
            FB2K_console_formatter()
                << "Last.fm: Extracted token (length: "
                << (int)token.length()
                << "), attempting authentication...";
        }

        // Show progress indicator during authentication
        NSAlert* progressAlert = [[NSAlert alloc] init];
        [progressAlert setMessageText:@"Authenticating..."];
        [progressAlert setInformativeText:@"Please wait while we verify your credentials with Last.fm."];
        [progressAlert setAlertStyle:NSAlertStyleInformational];

        __weak typeof(self) weakSelf = self;

        dispatch_async(dispatch_get_main_queue(), ^{
            [progressAlert layout];
            NSWindow* alertWindow = progressAlert.window;
            if (alertWindow) {
                [alertWindow setLevel:NSFloatingWindowLevel];
                [alertWindow orderFront:nil];
            }
        });

        // Perform asynchronous authentication
        foo_lastfm::g_lastfm_api->authenticate_async(token, [weakSelf, progressAlert](bool success) {
            dispatch_async(dispatch_get_main_queue(), ^{
                if (progressAlert.window) {
                    [progressAlert.window close];
                }
            });

            if (success) {
                std::string clean = foo_lastfm::g_lastfm_api->get_session_key();
                clean.erase(std::remove_if(clean.begin(), clean.end(),
                                           [](unsigned char c){ return std::isspace(c) || c == '\0'; }),
                            clean.end());
                foo_lastfm::cfg_session_key.set(clean.c_str());

                if (foo_lastfm::cfg_debug_enabled.get()) {
                    FB2K_console_formatter() << "Last.fm: Authentication SUCCESSFUL! Session key saved.";
                }

                dispatch_async(dispatch_get_main_queue(), ^{
                    NSAlert* successAlert = [[NSAlert alloc] init];
                    [successAlert setMessageText:@"✅ Success!"];
                    [successAlert setInformativeText:@"You're now authenticated with Last.fm!\n\nScrobbling will work automatically when you play music."];
                    [successAlert setAlertStyle:NSAlertStyleInformational];
                    [successAlert runModal];

                    [weakSelf updateStatusLabel];
                });
            } else {
                if (foo_lastfm::cfg_debug_enabled.get()) {
                    FB2K_console_formatter() << "Last.fm ERROR: Authentication FAILED";
                }

                dispatch_async(dispatch_get_main_queue(), ^{
                    NSAlert* errorAlert = [[NSAlert alloc] init];
                    [errorAlert setMessageText:@"❌ Authentication Failed"];
                    [errorAlert setInformativeText:@"Couldn't authenticate with Last.fm.\n\n"
                        "Please try again and make sure:\n"
                        "• You clicked 'Yes, allow access'\n"
                        "• You copied the URL AFTER the page reloaded\n"
                        "• Your API Key and Secret are correct\n\n"
                        "If it still doesn't work, try generating new API credentials."];
                    [errorAlert setAlertStyle:NSAlertStyleCritical];
                    [errorAlert runModal];
                });
            }
        });
    } else {
        if (foo_lastfm::cfg_debug_enabled.get()) {
            FB2K_console_formatter() << "Last.fm: Authentication cancelled by user";
        }
    }
}

@end

namespace foo_lastfm {
    class preferences_page_lastfm : public preferences_page {
    public:
        service_ptr instantiate() override {
            return fb2k::wrapNSObject([fooLastfmMacPreferences new]);
        }
        const char* get_name() override { return "Last.fm Scrobbler"; }
        GUID get_guid() override {
            return { 0x87654321, 0x4321, 0x4321, { 0x43, 0x21, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0 } };
        }
        GUID get_parent_guid() override { return guid_tools; }
    };

    static service_factory_t<preferences_page_lastfm> g_preferences_page_lastfm;
}
