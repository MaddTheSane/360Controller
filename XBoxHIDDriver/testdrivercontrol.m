/*
    This file is part of the Xbox HID Driver, Copyright (c) 2007 Darrell Walisser
    walisser@mac.com http://sourceforge.net/projects/xhd

    The Xbox HID Driver is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    The Xbox HID Driver is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Xbox HID Driver; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#import "DWXBoxHIDDriverInterface.h"
#import "DWXBoxHIDPrefsLoader.h"

static void showPadProperties(DWXBoxHIDDriverInterface *intf)
{
    printf("\tInverts Y axis: %d\n", [ intf invertsYAxis ]);
    printf("\tInverts X axis: %d\n", [ intf invertsXAxis ]);
    printf("\tInverts Ry axis: %d\n", [ intf invertsRyAxis ]);
    printf("\tInverts Rx axis: %d\n", [ intf invertsRxAxis ]);
    printf("\tClamps analog buttons: %d\n", [ intf clampsButtonValues ]);
    printf("\tClamps left trigger values:  %d\n", [ intf clampsLeftTriggerValues ]);
    printf("\tClamps right trigger values: %d\n", [ intf clampsRightTriggerValues ]);
    printf("\tMaps left trigger to button:  %d\n", [ intf mapsLeftTriggerToButton ]);
    printf("\tMaps right trigger to button: %d\n", [ intf mapsRightTriggerToButton ]);
    printf("\tLeft trigger threshold: %d\n", [ intf leftTriggerThreshold ]);
    printf("\tRight trigger threshold: %d\n", [ intf rightTriggerThreshold ]);
}

static void doMenuEditOptionsForPadDevice(DWXBoxHIDDriverInterface *intf)
{
    BOOL doIt = YES;
    int selection = 0;
    
    while (doIt) {
        
        printf( "---------------------------------\n");
        printf( "Current Properties for Pad\n");
        printf( "---------------------------------\n");
        showPadProperties(intf);
        
        printf( "---------------------------------\n"
                "Available Options For Pad\n"
                "---------------------------------\n"
                    "\t[0] toggle invert Y axis\n"
                    "\t[1] toggle invert X axis\n"
                    "\t[2] toggle invert Ry axis\n"
                    "\t[3] toggle invert Rx axis\n"
                    "\t[4] toggle button clamping\n"
                    "\t[5] toggle left trigger clamping\n"
                    "\t[6] toggle right trigger clamping\n"
                    "\t[7] toggle left trigger->button mapping\n"
                    "\t[8] toggle right trigger->button mapping\n"
                    "\t[9] set left trigger threshold value\n"
                    "\t[10] set right trigger threshold value\n"
                    "\t[11] save device options\n"
                    "\t[12] restore device options\n"
                    "\t[13] quit to main menu\n");
        
        printf("\n\nEnter selection [%d]: ", selection);
        fflush(stdout);
        
        fscanf(stdin, "%d", &selection);
        switch (selection) {
        case 0:
            [ intf setInvertsYAxis: ![ intf invertsYAxis ] ];
            break;
        case 1:
            [ intf setInvertsXAxis: ![ intf invertsXAxis ] ];
            break;
        case 2:
            [ intf setInvertsRyAxis: ![ intf invertsRyAxis ] ];
            break;
        case 3:
            [ intf setInvertsRxAxis: ![ intf invertsRxAxis ] ];
            break;
        case 4:
            [ intf setClampsButtonValues: ![ intf clampsButtonValues ] ];
            break;
        case 5:
            [ intf setClampsLeftTriggerValues: ![ intf clampsLeftTriggerValues ] ];
            break;
        case 6:
            [ intf setClampsRightTriggerValues: ![ intf clampsRightTriggerValues ] ];
            break;
        case 7:
            [ intf setMapsLeftTriggerToButton: ![ intf mapsLeftTriggerToButton ] ];
            break;
        case 8:
            [ intf setMapsRightTriggerToButton: ![ intf mapsRightTriggerToButton ] ];
            break;
        case 9:
            {
                int value;
                printf("Enter new value (0-255) [ current = %d ]: ", [ intf leftTriggerThreshold ]);
                fflush(stdout);
                if (1 == fscanf(stdin, "%d", &value))
                    [ intf setLeftTriggerThreshold:value ];
            }
            break;
        case 10:
            {
                int value;
                printf("Enter new value (0-255) [ current = %d ]: ", [ intf rightTriggerThreshold ]);
                fflush(stdout);
                if (1 == fscanf(stdin, "%d", &value))
                    [ intf setRightTriggerThreshold:value ];
            }
            break;
        case 11:
            [ DWXBoxHIDPrefsLoader saveConfigForDevice:intf ];
            break;
        case 12:
            [ DWXBoxHIDPrefsLoader loadSavedConfigForDevice:intf ];
            break;
        case 13:
            return;
        default:
            ;
        }
    }
}

int main(int argc, char**argv) {

    NSArray *interfaces;
    int i;
    int deviceIndex = 0;

    
    while (1) {

        NSAutoreleasePool *pool = [ [ NSAutoreleasePool alloc ] init ];
    
        printf ("--------------------------------\n");
        printf ("Listing Connected Devices\n");
        printf ("--------------------------------\n");
        
        interfaces = [ DWXBoxHIDDriverInterface interfaces ];
        
        for (i = 0; i < [ interfaces count ]; i++) {
        
        
            DWXBoxHIDDriverInterface *intf;
            
            intf = (DWXBoxHIDDriverInterface*)[ interfaces objectAtIndex:i ];
        
            printf ("Device %d\n", i+1);
            printf ("\tType: %s\n", [ [ intf deviceType ] cString ]);
            if ([ [ intf deviceType ] isEqualTo:NSSTR(kDeviceTypePadKey) ]) {
            
                showPadProperties(intf);
            }
        }
        
        if ([ interfaces count ] <= 0) {
            printf (">> No devices to control, exiting now...\n");
            [ pool release ];
            break;
        }
        
        printf ("\n\nselect a device to change options [%d]: ", deviceIndex);
        fflush(stdout);
        fscanf(stdin, "%d", &deviceIndex);
    
        if (deviceIndex > 0 && deviceIndex < [ interfaces count ] + 1) {
        
            DWXBoxHIDDriverInterface *intf;
            
            intf = (DWXBoxHIDDriverInterface*)[ interfaces objectAtIndex:deviceIndex-1 ];
            if ([ intf hasOptions ]) {
            
                if ([ [ intf deviceType ] isEqualTo:NSSTR(kDeviceTypePadKey) ]) {
            
                    doMenuEditOptionsForPadDevice(intf);
                }
            }
            else {
            
                printf(">> device has no options, pick another\n");
            }
        }
        else {
        
            deviceIndex = 0;
            printf(">> invalid device (%d), try again\n", deviceIndex);
        }

        
        [ pool release ];
        
        if (deviceIndex == 0)
            break;
    }
    
    return 0;
}