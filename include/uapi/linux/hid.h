/* SPDX-License-Identifier: GPL-2.0+ WITH Linux-syscall-note */
/*
 *  Copyright (c) 1999 Andreas Gal
 *  Copyright (c) 2000-2001 Vojtech Pavlik
 *  Copyright (c) 2006-2007 Jiri Kosina
 */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * Should you need to contact me, the author, you can do so either by
 * e-mail - mail your message to <vojtech@ucw.cz>, or by paper mail:
 * Vojtech Pavlik, Simunkova 1594, Prague 8, 182 00 Czech Republic
 */
#ifndef _UAPI__HID_H
#define _UAPI__HID_H



/*
 * USB HID (Human Interface Device) interface class code
 */

#define USB_INTERFACE_CLASS_HID		3

/*
 * USB HID interface subclass and protocol codes
 */

#define USB_INTERFACE_SUBCLASS_BOOT	1
#define USB_INTERFACE_PROTOCOL_KEYBOARD	1
#define USB_INTERFACE_PROTOCOL_MOUSE	2

/*
 * HID class requests
 */

#define HID_REQ_GET_REPORT		0x01
#define HID_REQ_GET_IDLE		0x02
#define HID_REQ_GET_PROTOCOL		0x03
#define HID_REQ_SET_REPORT		0x09
#define HID_REQ_SET_IDLE		0x0A
#define HID_REQ_SET_PROTOCOL		0x0B

/*
 * HID class descriptor types
 */

#define HID_DT_HID			(USB_TYPE_CLASS | 0x01)
#define HID_DT_REPORT			(USB_TYPE_CLASS | 0x02)
#define HID_DT_PHYSICAL			(USB_TYPE_CLASS | 0x03)

#define HID_MAX_DESCRIPTOR_SIZE		4096

/*
 * HID report item format
 */

#define HID_ITEM_FORMAT_SHORT			0
#define HID_ITEM_FORMAT_LONG			1

/*
 * Special tag indicating long items
 */

#define HID_ITEM_TAG_LONG			15

/*
 * HID report descriptor item type (prefix bit 2,3)
 */

#define HID_ITEM_TYPE_MAIN			0
#define HID_ITEM_TYPE_GLOBAL			1
#define HID_ITEM_TYPE_LOCAL			2
#define HID_ITEM_TYPE_RESERVED			3

/*
 * HID report descriptor main item tags
 */

#define HID_MAIN_ITEM_TAG_INPUT			8
#define HID_MAIN_ITEM_TAG_OUTPUT		9
#define HID_MAIN_ITEM_TAG_FEATURE		11
#define HID_MAIN_ITEM_TAG_BEGIN_COLLECTION	10
#define HID_MAIN_ITEM_TAG_END_COLLECTION	12

/*
 * HID report descriptor main item contents
 */

#define HID_MAIN_ITEM_CONSTANT			0x001
#define HID_MAIN_ITEM_VARIABLE			0x002
#define HID_MAIN_ITEM_RELATIVE			0x004
#define HID_MAIN_ITEM_WRAP			0x008
#define HID_MAIN_ITEM_NONLINEAR			0x010
#define HID_MAIN_ITEM_NO_PREFERRED		0x020
#define HID_MAIN_ITEM_NULL_STATE		0x040
#define HID_MAIN_ITEM_VOLATILE			0x080
#define HID_MAIN_ITEM_BUFFERED_BYTE		0x100

/*
 * HID report descriptor collection item types
 */

#define HID_COLLECTION_PHYSICAL			0
#define HID_COLLECTION_APPLICATION		1
#define HID_COLLECTION_LOGICAL			2
#define HID_COLLECTION_NAMED_ARRAY		4

/*
 * HID report descriptor global item tags
 */

#define HID_GLOBAL_ITEM_TAG_USAGE_PAGE		0
#define HID_GLOBAL_ITEM_TAG_LOGICAL_MINIMUM	1
#define HID_GLOBAL_ITEM_TAG_LOGICAL_MAXIMUM	2
#define HID_GLOBAL_ITEM_TAG_PHYSICAL_MINIMUM	3
#define HID_GLOBAL_ITEM_TAG_PHYSICAL_MAXIMUM	4
#define HID_GLOBAL_ITEM_TAG_UNIT_EXPONENT	5
#define HID_GLOBAL_ITEM_TAG_UNIT		6
#define HID_GLOBAL_ITEM_TAG_REPORT_SIZE		7
#define HID_GLOBAL_ITEM_TAG_REPORT_ID		8
#define HID_GLOBAL_ITEM_TAG_REPORT_COUNT	9
#define HID_GLOBAL_ITEM_TAG_PUSH		10
#define HID_GLOBAL_ITEM_TAG_POP			11

/*
 * HID report descriptor local item tags
 */

#define HID_LOCAL_ITEM_TAG_USAGE		0
#define HID_LOCAL_ITEM_TAG_USAGE_MINIMUM	1
#define HID_LOCAL_ITEM_TAG_USAGE_MAXIMUM	2
#define HID_LOCAL_ITEM_TAG_DESIGNATOR_INDEX	3
#define HID_LOCAL_ITEM_TAG_DESIGNATOR_MINIMUM	4
#define HID_LOCAL_ITEM_TAG_DESIGNATOR_MAXIMUM	5
#define HID_LOCAL_ITEM_TAG_STRING_INDEX		7
#define HID_LOCAL_ITEM_TAG_STRING_MINIMUM	8
#define HID_LOCAL_ITEM_TAG_STRING_MAXIMUM	9
#define HID_LOCAL_ITEM_TAG_DELIMITER		10

/*
 * HID usage tables
 */

#define HID_USAGE_PAGE				0xffff0000

#define HID_UP_UNDEFINED			0x00000000
#define HID_UP_GENDESK				0x00010000
#define HID_UP_SIMULATION			0x00020000
#define HID_UP_GENDEVCTRLS			0x00060000
#define HID_UP_KEYBOARD				0x00070000
#define HID_UP_LED				0x00080000
#define HID_UP_BUTTON				0x00090000
#define HID_UP_ORDINAL				0x000a0000
#define HID_UP_TELEPHONY			0x000b0000
#define HID_UP_CONSUMER				0x000c0000
#define HID_UP_DIGITIZER			0x000d0000
#define HID_UP_PID				0x000f0000
#define HID_UP_BATTERY				0x00850000
#define HID_UP_HPVENDOR				0xff7f0000
#define HID_UP_HPVENDOR2			0xff010000
#define HID_UP_MSVENDOR				0xff000000
#define HID_UP_CUSTOM				0x00ff0000
#define HID_UP_LOGIVENDOR			0xffbc0000
#define HID_UP_LOGIVENDOR2			0xff090000
#define HID_UP_LOGIVENDOR3			0xff430000
#define HID_UP_LNVENDOR				0xffa00000
#define HID_UP_SENSOR				0x00200000
#define HID_UP_ASUSVENDOR			0xff310000
#define HID_UP_GOOGLEVENDOR			0xffd10000

#define HID_USAGE				0x0000ffff

#define HID_GD_POINTER				0x00010001
#define HID_GD_MOUSE				0x00010002
#define HID_GD_JOYSTICK				0x00010004
#define HID_GD_GAMEPAD				0x00010005
#define HID_GD_KEYBOARD				0x00010006
#define HID_GD_KEYPAD				0x00010007
#define HID_GD_MULTIAXIS			0x00010008
/*
 * Microsoft Win8 Wireless Radio Controls extensions CA, see:
 * http://www.usb.org/developers/hidpage/HUTRR40RadioHIDUsagesFinal.pdf
 */
#define HID_GD_WIRELESS_RADIO_CTLS		0x0001000c
/*
 * System Multi-Axis, see:
 * http://www.usb.org/developers/hidpage/HUTRR62_-_Generic_Desktop_CA_for_System_Multi-Axis_Controllers.txt
 */
#define HID_GD_SYSTEM_MULTIAXIS			0x0001000e

#define HID_GD_X				0x00010030
#define HID_GD_Y				0x00010031
#define HID_GD_Z				0x00010032
#define HID_GD_RX				0x00010033
#define HID_GD_RY				0x00010034
#define HID_GD_RZ				0x00010035
#define HID_GD_SLIDER				0x00010036
#define HID_GD_DIAL				0x00010037
#define HID_GD_WHEEL				0x00010038
#define HID_GD_HATSWITCH			0x00010039
#define HID_GD_BUFFER				0x0001003a
#define HID_GD_BYTECOUNT			0x0001003b
#define HID_GD_MOTION				0x0001003c
#define HID_GD_START				0x0001003d
#define HID_GD_SELECT				0x0001003e
#define HID_GD_VX				0x00010040
#define HID_GD_VY				0x00010041
#define HID_GD_VZ				0x00010042
#define HID_GD_VBRX				0x00010043
#define HID_GD_VBRY				0x00010044
#define HID_GD_VBRZ				0x00010045
#define HID_GD_VNO				0x00010046
#define HID_GD_FEATURE				0x00010047
#define HID_GD_RESOLUTION_MULTIPLIER		0x00010048
#define HID_GD_SYSTEM_CONTROL			0x00010080
#define HID_GD_UP				0x00010090
#define HID_GD_DOWN				0x00010091
#define HID_GD_RIGHT				0x00010092
#define HID_GD_LEFT				0x00010093
/* Microsoft Win8 Wireless Radio Controls CA usage codes */
#define HID_GD_RFKILL_BTN			0x000100c6
#define HID_GD_RFKILL_LED			0x000100c7
#define HID_GD_RFKILL_SWITCH			0x000100c8

#define HID_DC_BATTERYSTRENGTH			0x00060020

#define HID_CP_CONSUMER_CONTROL			0x000c0001
#define HID_CP_AC_PAN				0x000c0238

#define HID_DG_DIGITIZER			0x000d0001
#define HID_DG_PEN				0x000d0002
#define HID_DG_LIGHTPEN				0x000d0003
#define HID_DG_TOUCHSCREEN			0x000d0004
#define HID_DG_TOUCHPAD				0x000d0005
#define HID_DG_WHITEBOARD			0x000d0006
#define HID_DG_STYLUS				0x000d0020
#define HID_DG_PUCK				0x000d0021
#define HID_DG_FINGER				0x000d0022
#define HID_DG_TIPPRESSURE			0x000d0030
#define HID_DG_BARRELPRESSURE			0x000d0031
#define HID_DG_INRANGE				0x000d0032
#define HID_DG_TOUCH				0x000d0033
#define HID_DG_UNTOUCH				0x000d0034
#define HID_DG_TAP				0x000d0035
#define HID_DG_TRANSDUCER_INDEX			0x000d0038
#define HID_DG_TABLETFUNCTIONKEY		0x000d0039
#define HID_DG_PROGRAMCHANGEKEY			0x000d003a
#define HID_DG_BATTERYSTRENGTH			0x000d003b
#define HID_DG_INVERT				0x000d003c
#define HID_DG_TILT_X				0x000d003d
#define HID_DG_TILT_Y				0x000d003e
#define HID_DG_TWIST				0x000d0041
#define HID_DG_TIPSWITCH			0x000d0042
#define HID_DG_TIPSWITCH2			0x000d0043
#define HID_DG_BARRELSWITCH			0x000d0044
#define HID_DG_ERASER				0x000d0045
#define HID_DG_TABLETPICK			0x000d0046
#define HID_DG_PEN_COLOR			0x000d005c
#define HID_DG_PEN_LINE_WIDTH			0x000d005e
#define HID_DG_PEN_LINE_STYLE			0x000d0070
#define HID_DG_PEN_LINE_STYLE_INK		0x000d0072
#define HID_DG_PEN_LINE_STYLE_PENCIL		0x000d0073
#define HID_DG_PEN_LINE_STYLE_HIGHLIGHTER	0x000d0074
#define HID_DG_PEN_LINE_STYLE_CHISEL_MARKER	0x000d0075
#define HID_DG_PEN_LINE_STYLE_BRUSH		0x000d0076
#define HID_DG_PEN_LINE_STYLE_NO_PREFERENCE	0x000d0077

#define HID_CP_CONSUMERCONTROL			0x000c0001
#define HID_CP_NUMERICKEYPAD			0x000c0002
#define HID_CP_PROGRAMMABLEBUTTONS		0x000c0003
#define HID_CP_MICROPHONE			0x000c0004
#define HID_CP_HEADPHONE			0x000c0005
#define HID_CP_GRAPHICEQUALIZER			0x000c0006
#define HID_CP_FUNCTIONBUTTONS			0x000c0036
#define HID_CP_SELECTION			0x000c0080
#define HID_CP_MEDIASELECTION			0x000c0087
#define HID_CP_SELECTDISC			0x000c00ba
#define HID_CP_VOLUMEUP				0x000c00e9
#define HID_CP_VOLUMEDOWN			0x000c00ea
#define HID_CP_PLAYBACKSPEED			0x000c00f1
#define HID_CP_PROXIMITY			0x000c0109
#define HID_CP_SPEAKERSYSTEM			0x000c0160
#define HID_CP_CHANNELLEFT			0x000c0161
#define HID_CP_CHANNELRIGHT			0x000c0162
#define HID_CP_CHANNELCENTER			0x000c0163
#define HID_CP_CHANNELFRONT			0x000c0164
#define HID_CP_CHANNELCENTERFRONT		0x000c0165
#define HID_CP_CHANNELSIDE			0x000c0166
#define HID_CP_CHANNELSURROUND			0x000c0167
#define HID_CP_CHANNELLOWFREQUENCYENHANCEMENT	0x000c0168
#define HID_CP_CHANNELTOP			0x000c0169
#define HID_CP_CHANNELUNKNOWN			0x000c016a
#define HID_CP_APPLICATIONLAUNCHBUTTONS		0x000c0180
#define HID_CP_GENERICGUIAPPLICATIONCONTROLS	0x000c0200

#define HID_DG_DEVICECONFIG			0x000d000e
#define HID_DG_DEVICESETTINGS			0x000d0023
#define HID_DG_AZIMUTH				0x000d003f
#define HID_DG_CONFIDENCE			0x000d0047
#define HID_DG_WIDTH				0x000d0048
#define HID_DG_HEIGHT				0x000d0049
#define HID_DG_CONTACTID			0x000d0051
#define HID_DG_INPUTMODE			0x000d0052
#define HID_DG_DEVICEINDEX			0x000d0053
#define HID_DG_CONTACTCOUNT			0x000d0054
#define HID_DG_CONTACTMAX			0x000d0055
#define HID_DG_SCANTIME				0x000d0056
#define HID_DG_SURFACESWITCH			0x000d0057
#define HID_DG_BUTTONSWITCH			0x000d0058
#define HID_DG_BUTTONTYPE			0x000d0059
#define HID_DG_BARRELSWITCH2			0x000d005a
#define HID_DG_TOOLSERIALNUMBER			0x000d005b
#define HID_DG_LATENCYMODE			0x000d0060

#define HID_BAT_ABSOLUTESTATEOFCHARGE		0x00850065

#define HID_VD_ASUS_CUSTOM_MEDIA_KEYS		0xff310076

/*
 * HID report types --- Ouch! HID spec says 1 2 3!
 */

#define HID_INPUT_REPORT	0
#define HID_OUTPUT_REPORT	1
#define HID_FEATURE_REPORT	2

#define HID_REPORT_TYPES	3


#endif /* _UAPI__HID_H */
