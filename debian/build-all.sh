#!/bin/bash
#
# Build chaosvpn.deb for all my chroot environments
#

set -e

git checkout debian/changelog
version="$( dpkg-parsechangelog --format dpkg | perl -ne '/^Version:\s+(.*)$/ && print "$1";' )"
[ -z "$version" ] && exit 1

git checkout debian/changelog
debchange --noquery --preserve --force-bad-version --newversion "${version}~sdinetG1" "Compiled $version for Debian Sid/Unstable"
derebuild sid

git checkout debian/changelog
debchange --noquery --preserve --force-bad-version --newversion "${version}~sdinetF1" "Compiled $version for Debian Wheezy"
derebuild wheezy

git checkout debian/changelog
debchange --noquery --preserve --force-bad-version --newversion "${version}~sdinetE1" "Compiled $version for Debian Squeeze"
derebuild squeeze

git checkout debian/changelog
debchange --noquery --preserve --force-bad-version --newversion "${version}~sdinetD1" "Compiled $version for Debian Lenny"
derebuild lenny

git checkout debian/changelog
exit 0
