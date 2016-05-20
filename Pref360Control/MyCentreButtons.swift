/*
	MICE Xbox 360 Controller driver for Mac OS X
	Copyright (C) 2006-2013 Colin Munro

	MyCentreButtons.swift - implementation of central button view

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

private func DrawButton(button: NSString, inRectangle rect: NSRect, pressed down: Bool) {
	let path = NSBezierPath(ovalInRect: rect)
	var size = NSZeroSize
	var attributes = [String: AnyObject]()
	var point = NSZeroPoint
	var colour = NSColor.blackColor()
	
	// Draw circle
	colour.set()
	if (down) {
		path.fill()
		colour = NSColor.whiteColor()
	} else {
		path.stroke()
	}
	// Draw text
	attributes[NSForegroundColorAttributeName] = colour
	size = button.sizeWithAttributes(attributes)
	point.x = rect.origin.x + ((rect.size.width - size.width) / 2);
	point.y = rect.origin.y + ((rect.size.height - size.height) / 2);
	button.drawAtPoint(point, withAttributes: attributes)
}

@IBDesignable
final class MyCentreButtons: NSView {
	@IBInspectable
	var back: Bool = false {
		didSet {
			self.needsDisplay = true
		}
	}
	@IBInspectable
	var start: Bool = false {
		didSet {
			self.needsDisplay = true
		}
	}
	@IBInspectable
	var specific: Bool = false {
		didSet {
			self.needsDisplay = true
		}
	}
	
	override func drawRect(dirtyRect: NSRect) {
		let area = self.bounds
		var button = NSZeroRect
		
		button.size.height = area.size.height / 2;
		button.size.width = area.size.width / 4;
		button.origin.x = area.origin.x;
		button.origin.y = area.origin.y + ((area.size.height - button.size.height) / 2);
		DrawButton("Back", inRectangle: button, pressed: back)
		button.origin.x = area.origin.x + area.size.width - button.size.width;
		DrawButton("Start", inRectangle: button, pressed: start)
		button.size.height = area.size.height - 2;
		button.size.width = button.size.height;
		button.origin.x = area.origin.x + ((area.size.width-button.size.width) / 2);
		button.origin.y = area.origin.y + 1;
		DrawButton("", inRectangle: button, pressed: specific)
	}
}
