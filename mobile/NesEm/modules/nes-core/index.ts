import NesCore from './src/NesCoreModule';
import { requireNativeViewManager } from 'expo-modules-core';

export function hello(): Promise<string> {
  if (!NesCore) return Promise.reject(new Error('dev build required'));
  return NesCore.hello();
}

export function loadRom(rom: Uint8Array) {
  if (!NesCore) return;
  NesCore.loadRom(rom);
}

export function tick(keys: number[]): Uint8Array | null {
  if (!NesCore) return null;
  const buf = NesCore.tick(new Uint8Array(keys));
  return new Uint8Array(buf);
}

export function shutdown() {
  if (!NesCore) return;
  NesCore.shutdown();
}

export const NesEmView = requireNativeViewManager('NesCore');
