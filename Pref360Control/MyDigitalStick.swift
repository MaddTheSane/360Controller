/*
	MICE Xbox 360 Controller driver for Mac OS X
	Copyright (C) 2006-2013 Colin Munro

	MyDigitalStick.swift - implementation of digital stick view

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

private let INSET_AMOUNT: CGFloat = 10

private func MakeTriangle(start: Int, inRectangle rect: NSRect) -> NSBezierPath {
	let path = NSBezierPath()
	var centre = NSZeroPoint
	var point = NSZeroPoint
	let mult: [[CGFloat]] = [[0,0],
		[1,0],
		[1,1],
		[0,1],
		[0,0]]
	
	// Find central part
	centre.x = rect.origin.x + (rect.size.width / 2);
	centre.y = rect.origin.y + (rect.size.height / 2);
	// Make triangle
	path.moveToPoint(centre)
	point.x = rect.origin.x + (rect.size.width * mult[start][0])
	point.y = rect.origin.y + (rect.size.height * mult[start][1])
	path.lineToPoint(point)
	point.x = rect.origin.x + (rect.size.width * mult[start + 1][0])
	point.y = rect.origin.y + (rect.size.height * mult[start + 1][1])
	path.lineToPoint(point)
	path.closePath()
	// Done
	return path;
}

@IBDesignable
final class MyDigitalStick: NSView {
	@IBInspectable
	var up: Bool = false {
		didSet {
			self.needsDisplay = true
		}
	}
	@IBInspectable
	var down: Bool = false {
		didSet {
			self.needsDisplay = true
		}
	}
	@IBInspectable
	var left: Bool = false {
		didSet {
			self.needsDisplay = true
		}
	}
	@IBInspectable
	var right: Bool = false {
		didSet {
			self.needsDisplay = true
		}
	}
	private var bezierUp: NSBezierPath!
	private var bezierDown: NSBezierPath!
	private var bezierLeft: NSBezierPath!
	private var bezierRight: NSBezierPath!

	private func setupBeziers() {
		let rect = self.bounds
		var triangle = NSZeroRect
		triangle.size.width = rect.size.width / 3;
		triangle.size.height = rect.size.height / 3;
		triangle.origin.y = rect.origin.y + (triangle.size.height * 2);
		triangle.origin.x = rect.origin.x + triangle.size.width;
		bezierUp = MakeTriangle(0, inRectangle: triangle)
		triangle.origin.y = rect.origin.y;
		bezierDown = MakeTriangle(2, inRectangle: triangle)
		triangle.origin.y = rect.origin.y + triangle.size.height;
		triangle.origin.x = rect.origin.x;
		bezierLeft = MakeTriangle(1, inRectangle: triangle)
		triangle.origin.x = rect.origin.x + (triangle.size.width * 2);
		bezierRight = MakeTriangle(3, inRectangle: triangle);
	}
	
	override init(frame: NSRect) {
		super.init(frame: frame)
		
		setupBeziers()
	}
	
	required init?(coder: NSCoder) {
		super.init(coder: coder)
		
		setupBeziers()
	}
	
    override func drawRect(dirtyRect: NSRect) {
		let area = self.bounds
		
		NSDrawLightBezel(area, area);
		NSColor.blackColor().set()
		
		if up { bezierUp.fill() }
		if down { bezierDown.fill() }
		if left { bezierLeft.fill() }
		if right { bezierRight.fill() }
    }
	
}
