//
//  KeyboardSceneDelegate.swift
//  Mini vMac
//
//  Created by Jesús A. Álvarez on 2024-02-10.
//  Copyright © 2024 namedfork. All rights reserved.
//

import UIKit

#if os(visionOS)
class KeyboardSceneDelegate: UIResponder, UIWindowSceneDelegate {
    var window: UIWindow?

    func scene(_ scene: UIScene, willConnectTo session: UISceneSession, options connectionOptions: UIScene.ConnectionOptions) {
        guard let windowScene = scene as? UIWindowScene else {
            fatalError("Expected scene of type UIWindowScene but got an unexpected type")
        }
        guard let mainViewController = AppDelegate.shared.window.rootViewController as? ViewController else {
            fatalError("No main view controller")
        }

        let defaultSize = mainViewController.keyboardViewController.preferredContentSize
        let minSize = defaultSize.applying(.init(scaleX: 0.75, y: 0.75))
        let maxSize = defaultSize.applying(.init(scaleX: 1.25, y: 1.25))
        windowScene.sizeRestrictions?.minimumSize = minSize
        windowScene.sizeRestrictions?.maximumSize = maxSize
        windowScene.requestGeometryUpdate(UIWindowScene.GeometryPreferences.Vision(
            size: defaultSize,
            minimumSize: minSize,
            maximumSize: maxSize,
            resizingRestrictions: .uniform
        ))
        window = UIWindow(windowScene: windowScene)

        if let window {
            window.rootViewController = mainViewController.keyboardViewController
            window.makeKeyAndVisible()
        }
    }
}
#endif
