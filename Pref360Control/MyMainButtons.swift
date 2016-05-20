/*
	MICE Xbox 360 Controller driver for Mac OS X
	Copyright (C) 2006-2013 Colin Munro

	MyMainButtons.swift - implementation of A/B/X/Y button view

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

private let MINI_OFFSET: CGFloat = 2

private func DrawButton(button: NSString, inRectangle rect: NSRect, pressed down: Bool) {
	var path = NSBezierPath(ovalInRect: rect)
	var size = NSZeroSize
	var point = NSZeroPoint
	let colour = NSColor.blackColor()
	let attributes = [NSForegroundColorAttributeName: colour]
	var bling = rect
	
	// Draw circle
	if(down) {
		path.fill()
	} else {
		path.stroke()
	}
	bling.insetInPlace(dx: -1, dy: -1)
	path = NSBezierPath(ovalInRect: bling)
	colour.set()
	path.stroke()

	// Draw text
	size = button.sizeWithAttributes(attributes)
	point.x = rect.origin.x + ((rect.size.width - size.width) / 2);
	point.y = rect.origin.y + ((rect.size.height - size.height) / 2);
	button.drawAtPoint(point, withAttributes: attributes)
}

@IBDesignable
final class MyMainButtons: NSView {
	@IBInspectable
	var a: Bool = false {
		didSet {
			self.needsDisplay = true
		}
	}
	@IBInspectable
	var b: Bool = false {
		didSet {
			self.needsDisplay = true
		}
	}
	@IBInspectable
	var x: Bool = false {
		didSet {
			self.needsDisplay = true
		}
	}
	@IBInspectable
	var y: Bool = false {
		didSet {
			self.needsDisplay = true
		}
	}
	
    override func drawRect(dirtyRect: NSRect) {
		let area = self.bounds
		var bit = NSZeroRect
		
		bit.size.width = area.size.width / 3;
		bit.size.height = area.size.height / 3;
		bit.origin.x = area.origin.x + bit.size.width;
		bit.origin.y = area.origin.y + (bit.size.height * 2) - MINI_OFFSET;
		NSColor.yellowColor().set()
		DrawButton("Y", inRectangle: bit, pressed: y)
		bit.origin.y = area.origin.y + MINI_OFFSET;
		NSColor.greenColor().set()
		DrawButton("A", inRectangle: bit, pressed: a)
		bit.origin.y = area.origin.y + bit.size.height;
		bit.origin.x = area.origin.x + MINI_OFFSET;
		NSColor.blueColor().set()
		DrawButton("X", inRectangle: bit, pressed: x)
		bit.origin.x = area.origin.x + (bit.size.width * 2) - MINI_OFFSET;
		NSColor.redColor().set()
		DrawButton("B", inRectangle: bit, pressed: b)
    }
	
}
