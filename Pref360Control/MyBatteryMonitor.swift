//
//  MyBatteryMonitor.swift
//  360 Driver
//
//  Created by C.W. Betts on 5/20/16.
//  Copyright © 2016 GitHub. All rights reserved.
//

import Cocoa

@IBDesignable
class MyBatteryMonitor: NSView {
    @IBInspectable
    var bars: Int32 = 0 {
        didSet {
            needsDisplay = true
        }
    }

    override func drawRect(dirtyRect: NSRect) {
        super.drawRect(dirtyRect)

        Pref360StyleKit.drawBatteryMonitorWithBars(CGFloat(bars))
    }
}
