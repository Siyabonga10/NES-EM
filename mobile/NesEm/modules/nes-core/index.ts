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

export function tick() {
  if (!NesCore) return;
  NesCore.tick();
}

export function getKeys(): number[] {
  if (!NesCore) return [0,0,0,0,0,0,0,0];
  const buf = NesCore.getKeys();
  return Array.from(new Uint8Array(buf));
}

export function shutdown() {
  if (!NesCore) return;
  NesCore.shutdown();
}

export const NesEmView = requireNativeViewManager('NesCore');
