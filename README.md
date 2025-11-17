<img src="docs/images/logo.png" alt="foo_mac_scrobble logo" title="foo_mac_scrobble" align="right" height="60px" />

# foo_mac_scrobble — Last.fm Scrobbler for foobar2000 macOS

[![Last Release](https://img.shields.io/github/v/release/avelytchko/foo_mac_scrobble?logo=github&label=latest&style=flat-square)](https://github.com/avelytchko/foo_mac_scrobble/releases)
[![Build](https://img.shields.io/github/actions/workflow/status/avelytchko/foo_mac_scrobble/build_foobar.yml?branch=main&logo=github&style=flat-square)](https://nightly.link/avelytchko/foo_mac_scrobble/workflows/build_foobar/main)
[![Downloads](https://img.shields.io/github/downloads/avelytchko/foo_mac_scrobble/total?logo=github&style=flat-square)](https://github.com/avelytchko/foo_mac_scrobble/releases/latest)

**Native Last.fm scrobbler for foobar2000 on macOS.** Automatically track your listening history with offline queueing, native preferences UI, and seamless Last.fm API integration. Built with the official foobar2000 SDK and Cocoa frameworks for a lightweight, reliable experience that respects your privacy and system resources.

---

## ✨ Features

- 🎧 **Automatic scrobbling** — tracks are submitted to Last.fm as you listen
- 📶 **Offline queueing** — stores plays locally when offline and syncs automatically when reconnected
- 🔐 **Secure authentication** — uses your own Last.fm API credentials with encrypted session keys
- ⚙️ **Native macOS UI** — fully integrated preferences panel with Cocoa interface
- 📊 **Configurable thresholds** — set when tracks should be scrobbled (percentage of playback)
- 🪲 **Built-in debugging** — optional console logging for troubleshooting
- 🚀 **Async networking** — non-blocking I/O keeps foobar2000 responsive
- 🪶 **Lightweight & open source** — minimal resource usage, MIT licensed

---

## 📦 Installation

### Via foobar2000 Preferences (Recommended)

1. Download the latest release from [**Releases**](../../releases)
2. Unzip the file to get `foo_mac_scrobble.component/`
3. Open **foobar2000 → Preferences → Components**
4. Click the **"+"** button and select the `foo_mac_scrobble.component` folder
5. Restart foobar2000

### Manual Installation

Copy the component folder to your user components directory:

```bash
~/Library/foobar2000-v2/user-components/foo_mac_scrobble/
```

Then restart foobar2000.

---

## ⚙️ Configuration

Open **foobar2000 → Preferences → Tools → Last.fm Scrobbler**

### Step 1: Get API Credentials

Visit [Last.fm API Account Creation](https://www.last.fm/api/account/create) and create an API account. You'll receive:
- **API Key**
- **API Secret**

Paste both into the preferences panel.

### Step 2: Authenticate

1. Click **Authenticate** — your browser will open to Last.fm
2. Approve access on the Last.fm authorization page
3. After approval, you'll see an "Invalid API key" error (this is expected)
4. Copy the **full URL** from your browser's address bar
5. Paste it into the component's dialog and press **OK**
6. You should see: ✅ **Authenticated as: yourusername**

### Step 3: Configure Preferences

- **Scrobble threshold:** Set the percentage of track completion required (default: 50%)
- **Debug logging:** Enable to see detailed output in foobar2000's Console

> 💡 **Tip:** Debug messages appear in **View → Console** and help diagnose authentication or network issues.

---

## 🔧 Quick Troubleshooting

### Scrobbles not appearing on Last.fm

- Check that you're authenticated (preferences should show your username)
- Enable debug logging and check the Console for error messages
- Verify your internet connection is active

### Need to re-authenticate

Delete these files and restart foobar2000:

```bash
~/Library/foobar2000-v2/lastfm_session.json
~/Library/foobar2000-v2/lastfm_scrobble_queue.json
```

Then go through the authentication steps again.

### Offline queue not syncing

The component automatically retries failed scrobbles when you're back online. If issues persist, check Console logs with debug mode enabled.

**More help:** See the [Wiki](../../wiki) for detailed troubleshooting, building from source, and technical documentation.

---

## 📸 Screenshots

**Preferences Panel**

<img src="docs/images/preferences.jpg" width="500px">

**Authentication Flow**

<img src="docs/images/auth_flow.jpg" width="400px">

---

## 💬 Support & Contributing

- **Report issues or Feature requests:** Use [GitHub Issues](../../issues) with the provided templates
- **Build from source:** See [Building Guide](../../wiki/Building-from-Source) in the Wiki
- **Contributing:** Pull requests welcome! Check [Contributing Guidelines](../../wiki/Contributing)

---

## 📄 License & Credits

**MIT License** © [Oleksandr Velychko](https://github.com/avelytchko)

Built with:
- [foobar2000 SDK](https://www.foobar2000.org/SDK)
- [Last.fm API](https://www.last.fm/api)
- Cocoa frameworks (macOS native)

**Enjoy the plugin?** Star the repo ⭐ or share feedback on [foobar2000 forums](https://www.foobar2000.org/forum)!

