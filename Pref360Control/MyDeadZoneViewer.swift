//
//  MyDeadZoneViewer.swift
//  360 Driver
//
//  Created by C.W. Betts on 5/20/16.
//  Copyright © 2016 GitHub. All rights reserved.
//

import Cocoa

class MyDeadZoneViewer: NSView {
    var val: Double = 0.0 {
        didSet {
            needsDisplay = true
        }
    }
    var linked: Bool = false {
        didSet {
            needsDisplay = true
        }
    }

    override func drawRect(dirtyRect: NSRect) {
        super.drawRect(dirtyRect)

        Pref360StyleKit.drawDeadZoneViewerWithValue(CGFloat(val / 32768.0), linked: linked)
    }
    
}
