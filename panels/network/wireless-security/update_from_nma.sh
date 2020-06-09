#!/bin/bash


WIRELESS_SECURITY_DIR=${top_srcdir}/../network-manager-applet/src/wireless-security

FILES="${NM_APPLET_SOURCES}" \
DIR="${WIRELESS_SECURITY_DIR}" \
${top_srcdir}/update-from-gsd.sh

patch -p4 < ${srcdir}/nm-connection-editor-to-network-panel.patch
git add ${NM_APPLET_SOURCES}
git commit -m "network: Update wireless-security from network-manager-applet"


FILES="${resource_files}" \
DIR="${WIRELESS_SECURITY_DIR}" \
${top_srcdir}/update-from-gsd.sh

patch -p4 < $(srcdir)/nm-connection-editor-ui-to-network-panel.patch
git add ${resource_files}
git commit -m "network: Update wireless-security UI from network-manager-applet"
