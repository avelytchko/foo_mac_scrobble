#!/usr/bin/env bash
#
# Bump the foo_mac_scrobble component version across every source location in
# one shot, so no spot gets missed.
#
# Canonical version locations (kept in sync by this script):
#   - foobar2000/foo_mac_scrobble/main.cpp        DECLARE_COMPONENT_VERSION(...)
#   - foobar2000/foo_mac_scrobble/lastfm_api.cpp  User-Agent string + debug log
#   - .../foo_mac_scrobble.xcodeproj/project.pbxproj  2x MARKETING_VERSION
#   - .github/ISSUE_TEMPLATE/bug_report.yml        version placeholder
#
# Usage:
#   scripts/bump-version.sh 0.1.4
#   scripts/bump-version.sh 0.1.4 --commit
#   scripts/bump-version.sh 0.1.4 --release "Fix preferences UI on Monterey"
#
#   (no flag)  edit files only — review & commit/tag yourself
#   --commit   also create the "Bump to version X.Y.Z" commit
#   --release  --commit, plus annotated tag vX.Y.Z and push branch + tag
#              (refuses if vX.Y.Z already exists, and requires a real branch)
#   --force    allow overwriting an already-existing vX.Y.Z tag (force-push)
#
# macOS / BSD sed only (matches the CI runner).
#
set -euo pipefail

die() { printf 'error: %b\n' "$1" >&2; exit 1; }

NEW="${1:-}"
[ -n "$NEW" ] || die "usage: $0 <new-version> [--commit | --release \"<message>\"]"
shift || true
[[ "$NEW" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]] || die "version must be X.Y.Z (got '$NEW')"

DO_COMMIT=0
DO_RELEASE=0
FORCE=0
MSG=""
while [ $# -gt 0 ]; do
  case "$1" in
    --commit)  DO_COMMIT=1 ;;
    --force)   FORCE=1 ;;
    --release) DO_RELEASE=1; DO_COMMIT=1; shift; MSG="${1:-}"
               [ -n "$MSG" ] || die "--release needs a message" ;;
    *) die "unknown arg: $1" ;;
  esac
  shift
done

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

PLUGIN="foobar2000/foo_mac_scrobble"
MAIN="$PLUGIN/main.cpp"
API="$PLUGIN/lastfm_api.cpp"
PBX="$PLUGIN/foo_mac_scrobble.xcodeproj/project.pbxproj"
BUG=".github/ISSUE_TEMPLATE/bug_report.yml"

for f in "$MAIN" "$API" "$PBX" "$BUG"; do
  [ -f "$f" ] || die "missing file: $f"
done

# Current version = single source of truth in main.cpp
CUR="$(grep -oE '"Last.fm Scrobbler", "[0-9]+\.[0-9]+\.[0-9]+"' "$MAIN" \
        | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -n1)"
[ -n "$CUR" ] || die "could not detect current version in $MAIN"
[ "$CUR" != "$NEW" ] || die "version is already $NEW"

CUR_RE="${CUR//./\\.}"   # escape dots for regex
echo "Bumping $CUR -> $NEW"

sed -i '' "s/\"Last.fm Scrobbler\", \"${CUR_RE}\"/\"Last.fm Scrobbler\", \"${NEW}\"/" "$MAIN"
sed -i '' "s#foo_mac_scrobble/${CUR_RE}#foo_mac_scrobble/${NEW}#g" "$API"
sed -i '' "s/MARKETING_VERSION = ${CUR_RE};/MARKETING_VERSION = ${NEW};/g" "$PBX"
sed -i '' "s/e\.g\., ${CUR_RE}/e.g., ${NEW}/" "$BUG"

# Fail loudly if any location still carries the old version
LEFT="$(grep -rnE "(\"Last.fm Scrobbler\", \"|foo_mac_scrobble/|MARKETING_VERSION = |e\.g\., )${CUR_RE}" \
          "$MAIN" "$API" "$PBX" "$BUG" || true)"
[ -z "$LEFT" ] || die "old version $CUR still present after edit:\n$LEFT"

echo "Updated:"
git diff --stat -- "$MAIN" "$API" "$PBX" "$BUG"

if [ "$DO_COMMIT" -eq 1 ]; then
  git add -- "$MAIN" "$API" "$PBX" "$BUG"
  git commit -m "Bump to version $NEW"
  echo "Committed: Bump to version $NEW"
fi

if [ "$DO_RELEASE" -eq 1 ]; then
  # Must be on a real branch, not a detached HEAD, before pushing.
  BRANCH="$(git symbolic-ref --quiet --short HEAD || true)"
  [ -n "$BRANCH" ] || die "detached HEAD — check out a branch before --release"

  # Refuse to clobber an existing release tag (local or remote) unless --force.
  TAG_EXISTS=0
  git rev-parse -q --verify "refs/tags/v$NEW" >/dev/null 2>&1 && TAG_EXISTS=1
  git ls-remote --exit-code --tags origin "v$NEW" >/dev/null 2>&1 && TAG_EXISTS=1
  if [ "$TAG_EXISTS" -eq 1 ] && [ "$FORCE" -eq 0 ]; then
    die "tag v$NEW already exists (local or origin); pass --force to overwrite a published release"
  fi

  if [ "$FORCE" -eq 1 ]; then
    git tag -fa "v$NEW" -m "$MSG"   # -f re-annotates an existing tag
    git push origin "$BRANCH"
    git push -f origin "v$NEW"      # force only when explicitly requested
  else
    git tag -a "v$NEW" -m "$MSG"
    git push origin "$BRANCH"
    git push origin "v$NEW"
  fi
  echo "Tagged & pushed v$NEW on $BRANCH"
fi

echo "Done."
