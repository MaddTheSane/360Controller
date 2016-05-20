//
//  MyWhole360Controller.swift
//  360 Driver
//
//  Created by C.W. Betts on 5/20/16.
//  Copyright © 2016 GitHub. All rights reserved.
//

import Cocoa

@IBDesignable
class MyWhole360Controller: NSView {
    @IBInspectable
    var aPressed: Bool = false {
        didSet {
            needsDisplay = true
        }
    }
    @IBInspectable
    var bPressed: Bool = false {
        didSet {
            needsDisplay = true
        }
    }
    @IBInspectable
    var xPressed: Bool = false {
        didSet {
            needsDisplay = true
        }
    }
    @IBInspectable
    var yPressed: Bool = false {
        didSet {
            needsDisplay = true
        }
    }
    
    @IBInspectable
    var leftPressed: Bool = false {
        didSet {
            needsDisplay = true
        }
    }
    @IBInspectable
    var upPressed: Bool = false {
        didSet {
            needsDisplay = true
        }
    }
    @IBInspectable
    var rightPressed: Bool = false {
        didSet {
            needsDisplay = true
        }
    }
    @IBInspectable
    var downPressed: Bool = false {
        didSet {
            needsDisplay = true
        }
    }
    
    @IBInspectable
    var backPressed: Bool = false {
        didSet {
            needsDisplay = true
        }
    }
    @IBInspectable
    var startPressed: Bool = false {
        didSet {
            needsDisplay = true
        }
    }
    @IBInspectable
    var homePressed: Bool = false {
        didSet {
            needsDisplay = true
        }
    }
    
    @IBInspectable
    var lbPressed: Bool = false {
        didSet {
            needsDisplay = true
        }
    }
    @IBInspectable
    var rbPressed: Bool = false {
        didSet {
            needsDisplay = true
        }
    }
    @IBInspectable
    var leftStickPressed: Bool = false {
        didSet {
            needsDisplay = true
        }
    }
    @IBInspectable
    var rightStickPressed: Bool = false {
        didSet {
            needsDisplay = true
        }
    }
    
    @IBInspectable
    var leftStickPosition: CGPoint = .zero {
        didSet {
            needsDisplay = true
        }
    }
    @IBInspectable
    var rightStickPosition: CGPoint = .zero {
        didSet {
            needsDisplay = true
        }
    }
    @IBInspectable
    var leftStickDeadzone: CGFloat {
        set(deadzone) {
            _leftStickDeadzone = deadzone / 32768.0;
            needsDisplay = true
        }
        get {
            return _leftStickDeadzone
        }
    }
    @IBInspectable
    var rightStickDeadzone: CGFloat {
        set(deadzone) {
            _rightStickDeadzone = deadzone / 32768.0;
            needsDisplay = true
        }
        get {
            return _rightStickDeadzone
        }
    }
    @IBInspectable
    var leftNormalized: Bool = false {
        didSet {
            needsDisplay = true
        }
    }
    @IBInspectable
    var rightNormalized: Bool = false {
        didSet {
            needsDisplay = true
        }
    }
    
    var leftStickXPos: Int32 = 0 {
        didSet {
            leftStickPosition.x = CGFloat(leftStickXPos) / 32768.0;
        }
    }
    var leftStickYPos: Int32 = 0 {
        didSet {
            leftStickPosition.y = CGFloat(leftStickYPos) / 32768.0;
        }
    }
    var rightStickXPos: Int32 = 0 {
        didSet {
            rightStickPosition.x = CGFloat(rightStickXPos) / 32768.0;
        }
    }
    var rightStickYPos: Int32 = 0 {
        didSet {
            rightStickPosition.y = CGFloat(rightStickYPos) / 32768.0;
        }
    }
    
    private var _rightStickDeadzone: CGFloat = 0
    private var _leftStickDeadzone: CGFloat = 0
    
    func reset() {
        aPressed = false
        bPressed = false
        xPressed = false
        yPressed = false
        leftPressed = false
        upPressed = false
        rightPressed = false
        downPressed = false
        backPressed = false
        startPressed = false
        homePressed = false
        lbPressed = false
        rbPressed = false
        leftStickPressed = false
        rightStickPressed = false
        leftStickPosition = .zero
        rightStickPosition = .zero
        leftStickDeadzone = 0
        rightStickDeadzone = 0
        leftNormalized = false
        rightNormalized = false
    }
    
    override func awakeFromNib() {
        super.awakeFromNib()
        leftStickPosition = NSZeroPoint
        rightStickPosition = NSZeroPoint
    }
    
    override func drawRect(dirtyRect: NSRect) {
        super.drawRect(dirtyRect)
        
        Pref360StyleKit.drawX360ControllerWithControllerNumber(0, aPressed: aPressed, bPressed: bPressed, xPressed: xPressed, yPressed: yPressed, leftPressed: leftPressed, upPressed: upPressed, rightPressed: rightPressed, downPressed: downPressed, backPressed: backPressed, startPressed: startPressed, lbPressed: lbPressed, rbPressed: rbPressed, homePressed: homePressed, leftStickPressed: leftStickPressed, rightStickPressed: rightStickPressed, leftStick: leftStickPosition, rightStick: rightStickPosition, leftStickDeadzone: leftStickDeadzone, rightStickDeadzone: rightStickDeadzone, isLeftNormalized: leftNormalized, isRightNormalized: rightNormalized)
    }
}
