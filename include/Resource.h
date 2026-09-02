#pragma once

// Resource Identifiers for MacTrafficLights

#define IDI_APP_ICON                  101

// Tray Menu Commands
#define IDM_TRAY_ENABLE               2001
#define IDM_TRAY_DISABLE              2002
#define IDM_TRAY_SETTINGS             2003
#define IDM_TRAY_DIAGNOSTICS          2004
#define IDM_TRAY_START_WITH_WINDOWS   2005
#define IDM_TRAY_ABOUT                2006
#define IDM_TRAY_EXIT                 2007

// Custom Window Messages
#define WM_APP_TRAYMSG                (WM_APP + 1)
#define WM_APP_UPDATE_DIAGNOSTIC      (WM_APP + 2)

// Settings Dialog Controls
#define IDD_SETTINGS                  3001
#define IDC_CHECK_ENABLED             3002
#define IDC_SLIDER_BUTTON_SIZE        3003
#define IDC_STATIC_BUTTON_SIZE        3004
#define IDC_SLIDER_SPACING            3005
#define IDC_STATIC_SPACING            3006
#define IDC_SLIDER_LEFT_MARGIN        3007
#define IDC_STATIC_LEFT_MARGIN        3008
#define IDC_SLIDER_TOP_MARGIN         3009
#define IDC_STATIC_TOP_MARGIN         3010
#define IDC_CHECK_DIM_INACTIVE        3011
#define IDC_CHECK_HOVER_SYMBOLS       3012
#define IDC_CHECK_STARTUP             3013
#define IDC_EDIT_EXCLUSIONS           3014
#define IDC_BTN_RESET_DEFAULTS        3015

// Diagnostics Dialog Controls
#define IDD_DIAGNOSTICS               4001
#define IDC_STATIC_CPU_USAGE          4002
#define IDC_STATIC_RAM_WORKING_SET    4003
#define IDC_STATIC_RAM_PRIVATE        4004
#define IDC_STATIC_TRACKED_WINDOWS    4005
#define IDC_STATIC_ACTIVE_OVERLAYS    4006
#define IDC_STATIC_HOOK_STATUS        4007
