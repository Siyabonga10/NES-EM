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

export function startLoop() {
  if (!NesCore) return;
  NesCore.startLoop();
}

export function stopLoop() {
  if (!NesCore) return;
  NesCore.stopLoop();
}

export function pauseLoop() {
  if (!NesCore) return;
  NesCore.pauseLoop();
}

export function resumeLoop() {
  if (!NesCore) return;
  NesCore.resumeLoop();
}

export function setVolume(v: number) {
  if (!NesCore) return;
  NesCore.setVolume(v);
}

export function setUncapped(u: boolean) {
  if (!NesCore) return;
  NesCore.setUncapped(u);
}

export function getFps(): number {
  if (!NesCore) return 0;
  return NesCore.getFps();
}

export const NesEmView = requireNativeViewManager('NesCore');
