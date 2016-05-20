//
//  MyShoulderButton.swift
//  360 Driver
//
//  Created by C.W. Betts on 11/7/14.
//  Copyright (c) 2014 GitHub. All rights reserved.
//

import Cocoa

private let INSET_AMOUNT: CGFloat = 2


@IBDesignable
class MyShoulderButton: NSView {
	@IBInspectable
	var pressed: Bool = false {
		didSet {
			self.needsDisplay = true
		}
	}
	
	override func drawRect(dirtyRect: NSRect) {
		var area = self.bounds
		
		NSDrawLightBezel(area, area);
		if (pressed) {
			area.origin.x += INSET_AMOUNT;
			area.origin.y += INSET_AMOUNT;
			area.size.width -= INSET_AMOUNT * 2;
			area.size.height -= INSET_AMOUNT * 2;
			NSColor.blueColor().set()
			NSRectFill(area);
		}
	}
}
