#include "stdafx.h"

// Declaration of your component's version information
// Since foobar2000 v1.0 having at least one of these in your DLL is mandatory to let the troubleshooter tell different versions of your component apart.
// Note that it is possible to declare multiple components within one DLL, but it's strongly recommended to keep only one declaration per DLL.
// As for 1.1, the version numbers are used by the component update finder to find updates; for that to work, you must have ONLY ONE declaration per DLL. If there are multiple declarations, the component is assumed to be outdated and a version number of "0" is assumed, to overwrite the component with whatever is currently on the site assuming that it comes with proper version numbers.
DECLARE_COMPONENT_VERSION(
    "Last.fm Scrobbler",
    "0.1",
    "Last.fm scrobbler for foobar2000 on macOS\n"
    "\n"
    "Automatically scrobbles tracks to Last.fm\n"
    "\n"
    "Features:\n"
    "- Automatic track scrobbling\n"
    "- Now Playing updates\n"
    "- Configurable scrobble threshold\n"
    "\n"
    "https://www.last.fm"
     "\n"
    "https://github.com/avelytchko/foo_mac_scrobble"
);


// This will prevent users from renaming your component around (important for proper troubleshooter behaviors) or loading multiple instances of it.
VALIDATE_COMPONENT_FILENAME("foo_mac_scrobble.component");

// Activate cfg_var downgrade functionality if enabled. Relevant only when cycling from newer FOOBAR2000_TARGET_VERSION to older.
FOOBAR2000_IMPLEMENT_CFG_VAR_DOWNGRADE;
