//
//  MyWhole360ControllerMapper.swift
//  360 Driver
//
//  Created by C.W. Betts on 5/20/16.
//  Copyright © 2016 GitHub. All rights reserved.
//

import Cocoa

private var mapping = [UInt8](count: 15, repeatedValue: 0)
private var previousMapping = [UInt8](count: 15, repeatedValue: 0)

private func realignButtonToByte(index: Int32) -> UInt8 {
    switch index {
    case 0:
        return 12
        
    case 1:
        return 13
        
    case 2:
        return 14
        
    case 3:
        return 15
        
    case 4:
        return 8
        
    case 5:
        return 9
        
    case 6:
        return 6
        
    case 7:
        return 7
        
    case 8:
        return 4
        
    case 9:
        return 5
        
    case 10:
        return 10
        
    case 11:
        return 0
        
    case 12:
        return 1
        
    case 13:
        return 2
        
    case 14:
        return 3
        
    default:
        return 0xff // Should Never Happen
    }
}

@IBDesignable
class MyWhole360ControllerMapper: MyWhole360Controller {
    var isMapping: Bool = false
    private var currentMappingIndex = 0
    private var remappingButton: NSButton?
    private var pref: Pref360ControlPref?

    class func mapping() -> UnsafeMutablePointer<UInt8> {
        return UnsafeMutablePointer(Pref360Control.mapping)
    }
    
    private func startMapping() {
        currentMappingIndex = 0;
        isMapping = true;
        upPressed = true
    }

    private func stopMapping() {
        super.reset()
        currentMappingIndex = 0;
        isMapping = false
        if let remappingButton = remappingButton {
            remappingButton.state = NSOffState
        }
        pref?.changeSetting(nil)
        BindingTableView.tableView().reloadData()
        pref?.changeSetting(nil)
    }

    private func saveCurrentMapping() {
        previousMapping = mapping
    }
    
    private func restorePreviousMapping() {
        mapping = previousMapping
    }

    func runMapperWithButton(button: NSButton!, andOwner prefPref: Pref360ControlPref!) {
        pref = prefPref;
        remappingButton = button;
        saveCurrentMapping()
        resetMapping()
        pref?.changeSetting(nil)
        startMapping()
    }
    
    func cancelMappingWithButton(button: NSButton!, andOwner prefPref: Pref360ControlPref!) {
        pref = prefPref;
        remappingButton = button;
        
        restorePreviousMapping()
        stopMapping()
    }
    
    func buttonPressedAtIndex(index: Int32) {
        mapping[currentMappingIndex] = realignButtonToByte(index)
        currentMappingIndex += 1
        
        setButtonAtIndex(currentMappingIndex)
        if (currentMappingIndex == 15) {
            stopMapping()
        }
    }
    
    private func setButtonAtIndex(index: Int) {
        switch (index) {
        case 1:
            upPressed = false
            downPressed = true
            
        case 2:
            downPressed = false
            leftPressed = true
            
        case 3:
            leftPressed = false
            rightPressed = true
            
        case 4:
            rightPressed = false
            startPressed = true
            
        case 5:
            startPressed = false
            backPressed = true
            
        case 6:
            backPressed = false
            leftStickPressed = true
            
        case 7:
            leftStickPressed = false
            rightStickPressed = true
            
        case 8:
            rightStickPressed = false
            lbPressed = true
            
        case 9:
            lbPressed = false
            rbPressed = true

        case 10:
            rbPressed = false
            homePressed = true
            
        case 11:
            homePressed = false
            aPressed = true
            
        case 12:
            aPressed = false
            bPressed = true
            
        case 13:
            bPressed = false
            xPressed = true
            
        case 14:
            xPressed = false
            yPressed = true
            
        case 15:
            yPressed = false
            
        default:
            break;
        }
    }
    
    private func resetMapping() {
        for i in 0..<15 {
            mapping[i] = UInt8(i)
        }
        for i in 12..<16 {
            mapping[i-1] = UInt8(i)
        }
    }

    func resetWithOwner(prefPref: Pref360ControlPref!) {
        pref = prefPref;
        reset()
    }

    override func reset() {
        guard (pref != nil) else {
        NSLog("Unable to reset remapping (pref is nil)");
            return
        }
        resetMapping()
        stopMapping()
    }
    
    override func awakeFromNib() {
        super.awakeFromNib()
        isMapping = false;
        resetMapping()
        BindingTableView.tableView().reloadData()
    }
}
