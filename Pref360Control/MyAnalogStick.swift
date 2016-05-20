/*
	MICE Xbox 360 Controller driver for Mac OS X
	Copyright (C) 2006-2013 Colin Munro

	MyAnalogStick.swift - implementation of analog stick view

	This file is part of Xbox360Controller.

	Xbox360Controller is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Xbox360Controller is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Foobar; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

import Cocoa

let PRESSED_INSET: CGFloat = 5
let AREA_INSET: CGFloat = 4

@IBDesignable
final class MyAnalogStick: NSView {
	@IBInspectable
	var deadzone: Int32 = 0 {
		didSet {
			self.needsDisplay = true
		}
	}
	@IBInspectable
	var positionX: Int32 = 0{
		didSet {
			if (normalized) {
				//const UInt16 max16 = 32767;
				let max16 = Float(UInt16.max)
				let maxVal: Float = Float(UInt16.max) - Float(deadzone)
				
				if (positionX > 0) {
				realPosition.x = Int32(Float(abs(positionX)) * maxVal / max16) + deadzone + 1
				} else if (positionX < 0) {
				realPosition.x = -(Int32(Float(abs(positionX)) * maxVal / max16) + deadzone + 1)
				} else {// x == 0
				realPosition.x = 0;
				}
			}
			else
			{
				realPosition.x = 0;
			}

			self.needsDisplay = true
		}
	}
	@IBInspectable
	var positionY: Int32 = 0{
		didSet {
			if (normalized) {
				//const UInt16 max16 = 32767;
				let max16 = Float(UInt16.max)
				let maxVal: Float = Float(UInt16.max) - Float(deadzone)
				
				if (positionY > 0) {
					realPosition.y = Int32(Float(abs(positionY)) * maxVal / max16) + deadzone + 1
				} else if (positionY < 0) {
					realPosition.y = -(Int32(Float(abs(positionY)) * maxVal / max16) + deadzone + 1)
				} else {// x == 0
					realPosition.y = 0;
				}
			} else {
				realPosition.y = 0;
			}
			self.needsDisplay = true
		}
	}
	@IBInspectable
	var pressed: Bool = false {
		didSet {
			self.needsDisplay = true
		}
	}
	@IBInspectable
	var linked: Bool = false {
		didSet {
			self.needsDisplay = true
		}
	}
	@IBInspectable
	var normalized: Bool = false {
		didSet {
			self.needsDisplay = true
		}
	}
	var realPosition: (x: Int32, y:Int32) = (0, 0)
	
	@objc(setPositionX:y:) func setPosition(x xPos: Int32, y yPos: Int32) {
		positionX = xPos
		positionY = yPos
	}
	
	override func drawRect(dirtyRect: NSRect) {
		let area = self.bounds
		var deadRect = NSZeroRect
		var posRect = NSZeroRect
		var realPosRect = NSZeroRect
		// Compute positions
		// Deadzone
		deadRect.size.width = (CGFloat(deadzone) * area.size.width) / 32768
		deadRect.size.height = (CGFloat(deadzone) * area.size.height) / 32768
		deadRect.origin.x = area.origin.x + ((area.size.width - deadRect.size.width) / 2);
		deadRect.origin.y = area.origin.y + ((area.size.height - deadRect.size.height) / 2);
		// Position
		posRect.size.width = 4;
		posRect.size.height = 4;
		posRect.origin.x = area.origin.x + AREA_INSET + ((CGFloat(positionX + 32768) * (area.size.width - (AREA_INSET * 2))) / 65536) - (posRect.size.width / 2);
		posRect.origin.y = area.origin.y + area.size.height - AREA_INSET - ((CGFloat(positionY + 32768) * (area.size.height - (AREA_INSET * 2))) / 65536) - (posRect.size.height / 2);
		// Real Position
		realPosRect.size.width = 4;
		realPosRect.size.height = 4;
		realPosRect.origin.x = area.origin.x + AREA_INSET + ((CGFloat(realPosition.x + 32768) * (area.size.width - (AREA_INSET * 2))) / 65536) - (posRect.size.width / 2);
		realPosRect.origin.y = area.origin.y + area.size.height - AREA_INSET - ((CGFloat(realPosition.y + 32768) * (area.size.height - (AREA_INSET * 2))) / 65536) - (posRect.size.height / 2);
		// Draw border
		NSDrawLightBezel(area, area);
		
		// Draw pressed state
		if pressed {
			var pressArea = area;
			
			pressArea.origin.x += PRESSED_INSET
			pressArea.origin.y += PRESSED_INSET
			pressArea.size.width -= PRESSED_INSET * 2
			pressArea.size.height -= PRESSED_INSET * 2
			NSColor.blackColor().set()
			NSRectFill(pressArea);
		}
		// Draw deadzone
		if (deadzone != 0) {
			NSColor.redColor().set()
			if (linked) {
				NSFrameRect(deadRect)
			} else {
				var trueRect = deadRect;
				
				trueRect.origin.x = area.origin.x;
				trueRect.size.width = area.size.width;
				NSFrameRect(trueRect);
				trueRect = deadRect;
				trueRect.origin.y = area.origin.y;
				trueRect.size.height = area.size.height;
				NSFrameRect(trueRect);
			}
		}
		// Draw real position
		if realPosition.x != 0 || realPosition.y != 0 {
			if pressed {
				NSColor(deviceWhite: 0.3, alpha: 1).set()
			} else {
				NSColor(deviceWhite: 0.7, alpha: 1).set()
			}
			NSRectFill(realPosRect);
		}
		// Draw position
		if (pressed) {
			NSColor.whiteColor().set()
		} else {
			NSColor.blackColor().set()
		}
		NSRectFill(posRect);
	}
}
