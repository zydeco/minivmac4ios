//
//  KeyboardSceneDelegate.swift
//  Mini vMac
//
//  Created by Jesús A. Álvarez on 2024-02-10.
//  Copyright © 2024 namedfork. All rights reserved.
//

import UIKit

class KeyboardSceneDelegate: UIResponder, UIWindowSceneDelegate {
    var window: UIWindow?

    func scene(_ scene: UIScene, willConnectTo session: UISceneSession, options connectionOptions: UIScene.ConnectionOptions) {
        guard let windowScene = scene as? UIWindowScene else {
            fatalError("Expected scene of type UIWindowScene but got an unexpected type")
        }
        guard let mainViewController = AppDelegate.shared.window.rootViewController as? ViewController else {
            fatalError("No main view controller")
        }

        let size = mainViewController.keyboardViewController.preferredContentSize
        windowScene.sizeRestrictions?.minimumSize = size
        windowScene.sizeRestrictions?.maximumSize = size
        window = UIWindow(windowScene: windowScene)

        if let window {
            window.rootViewController = mainViewController.keyboardViewController
            window.makeKeyAndVisible()
        }
    }
}
