//
//  SettingsMenu.swift
//  Mini vMac
//
//  Created by Jesús A. Álvarez on 2024-02-10.
//  Copyright © 2024 namedfork. All rights reserved.
//

import SwiftUI

#if os(visionOS)
struct SettingsMenu: View {
    var body: some View {
        Menu() {
            Section("Settings") {
                SpeedMenu()
                MachineMenu()
                DisplayScalingMenu()
            }
        } label: {
            Image(systemName: "gear")
        }.menuOrder(.fixed)
    }
}

struct SpeedMenu: View {
    @AppStorage("speedValue") var currentSpeed: EmulatorSpeed = .speed1x
    @AppStorage("runInBackground") var runInBackground: Bool = false
    private var currentSpeedImage: String {
        switch currentSpeed {
        case .speed1x:
            "tortoise"
        case .speedAllOut:
            "hare"
        case .speed2x:
            "2.square"
        case .speed4x:
            "4.square"
        case .speed8x:
            "8.square"
        case .speed16x:
            "16.square"
        case .speed32x:
            "32.square"
        @unknown default:
            "hare"
        }
    }
    var body: some View {
        Menu("Speed", systemImage:currentSpeedImage) {
            SpeedButton(label: "1x", speed: .speed1x)
            SpeedButton(label: "2x", speed: .speed2x)
            SpeedButton(label: "4x", speed: .speed4x)
            SpeedButton(label: "8x", speed: .speed8x)
            SpeedButton(label: "16x", speed: .speed16x)
            SpeedButton(label: "32x", speed: .speed32x)
            SpeedButton(label: "Unlimited", speed: .speedAllOut)
            Divider()
            Toggle("Run in Background", isOn: $runInBackground)
        }.menuOrder(.fixed)
    }
}

struct SpeedButton: View {
    @AppStorage("speedValue") var currentSpeed: EmulatorSpeed = .speed1x
    let label: LocalizedStringKey
    let speed: EmulatorSpeed
    var body: some View {
        Toggle(label, isOn: Binding(get: {
            currentSpeed == speed
        }, set: { enable in
            if enable {
                currentSpeed = speed
            }
        }))
    }
}

struct MachineMenu: View {
    @AppStorage("machine") var currentMachine: String = "Plus4M"
    let bundles = AppDelegate.shared.emulatorBundles?.sorted(by: { $0.bundleIdentifier! < $1.bundleIdentifier! }) ?? []
    var body: some View {
        Menu("Machine", systemImage: "desktopcomputer") {
            ForEach(bundles, id: \.bundleIdentifier) { bundle in
                MachineButton(bundle: bundle)
            }
        }.menuOrder(.fixed)
    }
}

struct MachineButton: View {
    @AppStorage("machine") var currentMachine: String = "Plus4M"
    var bundle: Bundle
    var body: some View {
        Toggle(isOn: Binding(get: {
            currentMachine == bundle.name
        }, set: { enable in
            if enable && !AppDelegate.emulator.anyDiskInserted {
                currentMachine = bundle.name
                AppDelegate.shared.loadAndStartEmulator()
            }
        }), label: {
            Text("\(bundle.displayName ?? bundle.name)\n\(bundle.getInfoString ?? "")")
        })
    }
}

fileprivate extension Bundle {
    var name: String { ((self.bundlePath as NSString).lastPathComponent as NSString).deletingPathExtension }
    var displayName: String? { self.object(forInfoDictionaryKey: "CFBundleDisplayName") as? String }
    var getInfoString: String? { self.object(forInfoDictionaryKey: "CFBundleGetInfoString") as? String }
}

struct DisplayScalingMenu: View {
    @AppStorage("screenFilter") var scalingFilter: CALayerContentsFilter = .nearest
    static let filters = [
        DisplayScalingFilter(filter: .nearest, name: "Nearest"),
        DisplayScalingFilter(filter: .linear, name: "Linear"),
        DisplayScalingFilter(filter: .trilinear, name: "Trilinear"),
    ]
    var body: some View {
        Menu("Display Scaling", systemImage: "rectangle.and.text.magnifyingglass") {
            ForEach(DisplayScalingMenu.filters, id: \.filter) { filter in
                Toggle(filter.name, isOn: Binding(get: {
                    scalingFilter == filter.filter
                }, set: { enable in
                    if enable {
                        scalingFilter = filter.filter
                    }
                }))
            }
        }.menuOrder(.fixed)
    }
}

struct DisplayScalingFilter {
    let filter: CALayerContentsFilter
    let name: String
}

#endif
