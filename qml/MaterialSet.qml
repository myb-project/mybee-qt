pragma Singleton
import QtQml 2.15
import QtQuick.Controls.Material 2.15

QtObject {
    readonly property int defaultTheme: Material.Dark
    readonly property var themeColor: [
        {
            // 0 - Material.Light
            "accent":       "#5f9ea0", // cadetBlue
            "background":   "#eff0f1",
            "highlight":    Qt.darker("#eff0f1", 1.1),
            "backlight":    Qt.lighter("#eff0f1", 1.2),
            "foreground":   "#0f0f0f",
            "primary":      Material.Grey,
            "shadePrimary": Material.color(Material.Grey, Material.Shade400)
        },{
            // 1 - Material.Dark
            "accent":       "#5f9ea0", // cadetBlue
            "background":   "#4b4d4f",
            "highlight":    Qt.lighter("#4b4d4f", 1.1),
            "backlight":    Qt.darker("#4b4d4f", 1.2),
            "foreground":   "#f0f0f0",
            "primary":      Material.BlueGrey,
            "shadePrimary": Material.color(Material.BlueGrey, Material.Shade300)
        },{
            // 2 - Material.System
            "accent":       Material.accent,
            "background":   Material.background,
            "highlight":    Qt.darker(Material.background, 1.1),
            "backlight":    Qt.lighter(Material.background, 1.2),
            "foreground":   Material.foreground,
            "primary":      Material.primary,
            "shadePrimary": Material.primary
        }
    ]
}
