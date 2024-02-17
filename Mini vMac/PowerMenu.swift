//
//  PowerMenu.swift
//  Mini vMac
//
//  Created by Jesús A. Álvarez on 2024-02-10.
//  Copyright © 2024 namedfork. All rights reserved.
//

import SwiftUI

#if os(visionOS)
struct PowerMenu: View {
    var body: some View {
        Menu() {
            Section("Power") {
                Button("Restart", systemImage: "arrowtriangle.right", role: .destructive) {
                    AppDelegate.shared.loadAndStartEmulator()
                }
                Button("Interrupt", systemImage: "arrowtriangle.down.circle", role: .destructive) {
                    AppDelegate.emulator.interrupt()
                }
                Button("Shut Down", systemImage: "minus.circle", role: .destructive) {
                    exit(0)
                }
            }
        } label: {
            Image(systemName: "power")
        }.menuOrder(.fixed)
    }
}

#Preview {
    PowerMenu()
}
#endif
