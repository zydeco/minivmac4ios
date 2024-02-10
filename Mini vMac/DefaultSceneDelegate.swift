//
//  DefaultSceneDelegate.swift
//  Mini vMac
//
//  Created by Jesús A. Álvarez on 2024-02-09.
//  Copyright © 2024 namedfork. All rights reserved.
//

import UIKit

class DefaultSceneDelegate: UIResponder, UIWindowSceneDelegate {
    var window: UIWindow?

    func scene(_ scene: UIScene, willConnectTo session: UISceneSession, options connectionOptions: UIScene.ConnectionOptions) {
        guard let windowScene = scene as? UIWindowScene else {
            fatalError("Expected scene of type UIWindowScene but got an unexpected type")
        }
        guard let appDelegate = AppDelegate.sharedInstance() else {
            fatalError("No app delegate")
        }

        let size = CGSize(width: 1024.0, height: 684.0)
        windowScene.sizeRestrictions?.minimumSize = size
        windowScene.sizeRestrictions?.maximumSize = size

        window = UIWindow(windowScene: windowScene)
        let rootViewController = appDelegate.window?.rootViewController ?? UIStoryboard(name: "Main", bundle: .main).instantiateInitialViewController()

        if let window {
            appDelegate.window = window
            window.rootViewController = rootViewController
            window.makeKeyAndVisible()
        }
    }
}
