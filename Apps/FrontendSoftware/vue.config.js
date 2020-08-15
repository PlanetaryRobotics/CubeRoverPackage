// vue.config.js

module.exports = {
  pluginOptions: {
    electronBuilder: {
      builderOptions: {
        // options placed here will be merged with default configuration and passed to electron-builder
        "appId": "com.iris.terminal",
        "copyright": "Copyright © 2020 ${author}",
        "mac": {
          "target": "dmg",
          "icon": "build/icon.png"
        },
        "dmg": {
          "title": "${productName} ${version}",
          "background": "build/background.png",
          "icon": "build/icon.png",
          "contents": [
            {
              "x": 320,
              "y": 320
            },
            {
              "x": 240,
              "y": 150,
              "type": "link",
              "path": "/Applications"
            }
          ]
        },
        "win": {
          "target": "nsis",
          "icon": "build/icon.png",
          "legalTrademarks": "Copyright © 2020 ${author}"
        },
        "linux": {
          "target": [
            "AppImage",
            "deb"
          ]
        }
      }
    }
  }
}
