import { requireNativeModule } from 'expo-modules-core';

let NesCore: any = null;

try {
  NesCore = requireNativeModule('NesCore');
} catch {
  // Native module only available in development builds (npx expo run:android)
}

export default NesCore;
