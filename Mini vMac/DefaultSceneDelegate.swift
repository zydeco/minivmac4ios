//
//  DefaultSceneDelegate.swift
//  Mini vMac
//
//  Created by Jesús A. Álvarez on 2024-02-09.
//  Copyright © 2024 namedfork. All rights reserved.
//

import UIKit

class DefaultSceneDelegate: UIResponder, UIWindowSceneDelegate {
    var window: UIWindow? // keep window reference to be able to set background colour before destroying

    func scene(_ scene: UIScene, willConnectTo session: UISceneSession, options connectionOptions: UIScene.ConnectionOptions) {
        guard let windowScene = scene as? UIWindowScene else {
            fatalError("Expected scene of type UIWindowScene but got an unexpected type")
        }
        guard let appDelegate = AppDelegate.shared else {
            fatalError("No app delegate")
        }

        let size = CGSize(width: 1024.0, height: 684.0)
        windowScene.sizeRestrictions?.minimumSize = size
        windowScene.sizeRestrictions?.maximumSize = size

        window = UIWindow(windowScene: windowScene)
        if let window {
            appDelegate.window = window
            window.rootViewController = UIStoryboard(name: "Main", bundle: .main).instantiateInitialViewController()
            window.makeKeyAndVisible()
        }
        self.destroyOtherSessions(not: session)
    }

    private func destroyOtherSessions(not session: UISceneSession) {
        let app = UIApplication.shared
        let options = UIWindowSceneDestructionRequestOptions()
        options.windowDismissalAnimation = .decline
        for otherSession in app.openSessions.filter({ $0 != session && $0.configuration.name == "Default"}) {
            if let window = (otherSession.scene as? UIWindowScene)?.windows.first {
                window.rootViewController?.view.removeFromSuperview()
                window.backgroundColor = .darkGray
                app.requestSceneSessionRefresh(otherSession)
            }
            app.requestSceneSessionDestruction(otherSession, options: options)
            // window will remain visible until window switcher is dismissed!
        }
    }

    func sceneDidEnterBackground(_ scene: UIScene) {
        let app = UIApplication.shared
        if UserDefaults.standard.bool(forKey: "runInBackground") == false && app.connectedScenes.filter({ $0 != scene && $0.session.configuration.name == "Default"}).isEmpty {
            AppDelegate.emulator.isRunning = false
        }
    }

    func sceneDidBecomeActive(_ scene: UIScene) {
        AppDelegate.emulator.isRunning = true
    }

    func windowScene(_ windowScene: UIWindowScene, performActionFor shortcutItem: UIApplicationShortcutItem, completionHandler: @escaping (Bool) -> Void) {
        AppDelegate.shared.application(UIApplication.shared, performActionFor: shortcutItem, completionHandler: completionHandler)
    }
}
