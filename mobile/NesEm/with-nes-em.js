const { withAppBuildGradle, withGradleProperties } = require('@expo/config-plugins');
const path = require('path');

module.exports = function withNesEm(config) {
  config = withGradleProperties(config, (cfg) => {
    cfg.modResults.push(
      { type: 'property', key: 'reactNativeArchitectures', value: 'arm64-v8a' },
      { type: 'property', key: 'android.enableMinifyInReleaseBuilds', value: 'true' },
      { type: 'property', key: 'android.enableShrinkResourcesInReleaseBuilds', value: 'true' },
    );
    return cfg;
  });

  config = withAppBuildGradle(config, (cfg) => {
    const ks = path.join(cfg.modRequest.projectRoot, 'release.keystore').replace(/\\/g, '/');

    /* Add release signing config after the debug block */
    const signBlock = `
signingConfigs {
        debug {
            storeFile file('debug.keystore')
            storePassword 'android'
            keyAlias 'androiddebugkey'
            keyPassword 'android'
        }
        release {
            storeFile file('${ks}')
            storePassword 'android1'
            keyAlias 'nesem'
            keyPassword 'android1'
        }
}`;
    cfg.modResults.contents = cfg.modResults.contents.replace(
      /signingConfigs\s*\{[^}]*debug\s*\{[^}]*\}\s*\}/s,
      signBlock,
    );

    /* Point release buildType to release signing */
    cfg.modResults.contents = cfg.modResults.contents.replace(
      /(release\s*\{[^}]*signingConfig\s+)signingConfigs\.debug/,
      '$1signingConfigs.release',
    );
    return cfg;
  });

  return config;
};
