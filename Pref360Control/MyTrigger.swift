//
//  MyTrigger.swift
//  360 Driver
//
//  Created by C.W. Betts on 5/20/16.
//  Copyright © 2016 GitHub. All rights reserved.
//

import Cocoa

@IBDesignable
class MyTrigger: NSView {
    @IBInspectable
    var name: String = "" {
        didSet {
            needsDisplay = true
        }
    }
    @IBInspectable
    var val: Int32 = 0 {
        didSet {
            needsDisplay = true
        }
    }

    override func drawRect(dirtyRect: NSRect) {
        super.drawRect(dirtyRect)
        
        Pref360StyleKit.drawTriggerMetterWithIntensity(CGFloat(val) / 255.0, triggerTitle: name)
    }    
}
