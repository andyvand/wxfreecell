#!/bin/sh
set -e
PLIST="$1"
/usr/bin/plutil -remove CFBundleHelpBookName   "$PLIST" 2>/dev/null || true
/usr/bin/plutil -remove CFBundleHelpBookFolder "$PLIST" 2>/dev/null || true
/usr/bin/plutil -insert CFBundleHelpBookName   -string "wxfreecell Help"  "$PLIST"
/usr/bin/plutil -insert CFBundleHelpBookFolder -string "wxfreecell.help" "$PLIST"
