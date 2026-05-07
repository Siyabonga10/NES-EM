import { View, Text, Pressable, StyleSheet, Alert, AppState } from 'react-native';
import { useEffect, useState, useRef, useCallback } from 'react';
import * as DocumentPicker from 'expo-document-picker';
import * as FileSystem from 'expo-file-system/legacy';
import { hello, loadRom, tick, getKeys, shutdown, NesEmView } from '@/modules/nes-core';

const KEYS = ['a','b','up','down','left','right','start','select'] as const;
const CHARS = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';

function b64decode(input: string): Uint8Array {
  const s = input.replace(/[^A-Za-z0-9+/=]/g, '');
  const out: number[] = [];
  for (let i = 0; i < s.length; i += 4) {
    const a = CHARS.indexOf(s[i]), b = CHARS.indexOf(s[i + 1]);
    const c = CHARS.indexOf(s[i + 2]), d = CHARS.indexOf(s[i + 3]);
    out.push((a << 2) | (b >> 4));
    if (c !== 64) out.push(((b & 15) << 4) | (c >> 2));
    if (d !== 64) out.push(((c & 3) << 6) | d);
  }
  return new Uint8Array(out);
}

export default function HomeScreen() {
  const [loaded, setLoaded] = useState(false);
  const [romLoaded, setRomLoaded] = useState(false);
  const [frameN, setFrameN] = useState(0);
  const [paused, setPaused] = useState(false);
  const [pressedKeys, setPressedKeys] = useState<Set<string>>(new Set());
  const timerRef = useRef<ReturnType<typeof setInterval> | null>(null);
  const pausedRef = useRef(false);
  const appStateRef = useRef(AppState.currentState);

  const stopLoop = useCallback(() => {
    if (timerRef.current) { clearInterval(timerRef.current); timerRef.current = null; }
  }, []);

  const startLoop = useCallback(() => {
    if (timerRef.current) return;
    timerRef.current = setInterval(() => {
      if (pausedRef.current) return;
      tick();
      const k = getKeys();
      const next = new Set<string>();
      for (let i = 0; i < 8; i++) { if (k[i]) next.add(KEYS[i]); }
      setPressedKeys(next);
      setFrameN(n => n + 1);
    }, 16);
  }, []);

  const togglePause = useCallback(() => {
    setPaused(p => { pausedRef.current = !p; return !p; });
  }, []);

  useEffect(() => {
    hello().then(() => setLoaded(true)).catch((e) => console.log('[emu] err', e));
    const sub = AppState.addEventListener('change', (nextState) => {
      if (appStateRef.current === 'active' && nextState.match(/inactive|background/)) {
        pausedRef.current = true;
        setPaused(true);
      }
      appStateRef.current = nextState;
    });
    return () => { stopLoop(); shutdown(); sub.remove(); };
  }, [stopLoop]);

  const handleLoadRom = useCallback(async () => {
    try {
      const r = await DocumentPicker.getDocumentAsync({ type: '*/*', copyToCacheDirectory: true });
      if (r.canceled || !r.assets?.length) return;
      const b64 = await FileSystem.readAsStringAsync(r.assets[0].uri, { encoding: FileSystem.EncodingType.Base64 });
      if (!b64) { Alert.alert('Error', 'Failed to read file'); return; }
      const bytes = b64decode(b64);
      let ck = 0;
      for (let i = 0; i < bytes.length; i++) ck ^= bytes[i];
      console.log('[emu] ROM: ' + bytes.length + 'b, xor=' + ck.toString(16));
      stopLoop();
      shutdown();
      loadRom(bytes);
      setRomLoaded(true);
      setPaused(false);
      pausedRef.current = false;
      startLoop();
    } catch (e: any) { Alert.alert('Error', e.message); }
  }, [startLoop, stopLoop]);

  const btnColor = (key: string) => pressedKeys.has(key) ? '#4a4a4a' : '#2a2a2a';

  return (
    <View style={styles.root}>
      {/* Native touch overlay — full screen, zIndex below controls */}
      <NesEmView style={styles.touchOverlay} />

      {/* Visual controls — no touch handlers, pointer-events pass through */}
      <View style={styles.dpad} pointerEvents="none">
        <View style={[styles.dbtn, styles.dUp, { backgroundColor: btnColor('up') }]}>
          <Text style={styles.dText}>▲</Text>
        </View>
        <View style={[styles.dbtn, styles.dDown, { backgroundColor: btnColor('down') }]}>
          <Text style={styles.dText}>▼</Text>
        </View>
        <View style={[styles.dbtn, styles.dLeft, { backgroundColor: btnColor('left') }]}>
          <Text style={styles.dText}>◄</Text>
        </View>
        <View style={[styles.dbtn, styles.dRight, { backgroundColor: btnColor('right') }]}>
          <Text style={styles.dText}>►</Text>
        </View>
      </View>

      <View style={styles.actions} pointerEvents="none">
        <View style={[styles.actBtn, { backgroundColor: btnColor('b') }]}>
          <Text style={styles.actText}>B</Text>
        </View>
        <View style={[styles.actBtn, { backgroundColor: btnColor('a') }]}>
          <Text style={styles.actText}>A</Text>
        </View>
      </View>

      {/* Center game area — this is where the second NesEmView would render, but we already have it as full-screen overlay.
           The game image draws centered within the touch overlay. */}
      <View style={styles.center} pointerEvents="box-none">
        {!romLoaded && (
          <Pressable style={styles.loadBtn} onPress={handleLoadRom}>
            <Text style={styles.loadText}>Load ROM</Text>
          </Pressable>
        )}
      </View>

      {/* Top bar */}
      <View style={styles.topBar}>
        <Text style={styles.fps}>F#{frameN} ctrl={loaded ? 'OK' : 'NO'}{paused ? ' ⏸' : ''}</Text>
        <View style={styles.topActions}>
          {romLoaded && (
            <Pressable style={styles.topBtn} onPress={togglePause}>
              <Text style={styles.topBtnText}>{paused ? '▶' : '⏸'}</Text>
            </Pressable>
          )}
          <Pressable style={styles.topBtn} onPress={handleLoadRom}>
            <Text style={styles.topBtnText}>ROM</Text>
          </Pressable>
        </View>
      </View>

      {/* Start / Select */}
      <View style={[styles.corner, styles.sel, { backgroundColor: btnColor('select') }]} pointerEvents="none">
        <Text style={styles.cornerText}>Sel</Text>
      </View>
      <View style={[styles.corner, styles.sta, { backgroundColor: btnColor('start') }]} pointerEvents="none">
        <Text style={styles.cornerText}>Start</Text>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  root: { flex: 1, backgroundColor: '#111' },
  touchOverlay: { position: 'absolute', top: 0, left: 0, right: 0, bottom: 0, zIndex: 5 },
  topBar: { position: 'absolute', top: 0, left: 0, right: 0, flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', zIndex: 10, paddingHorizontal: 8, paddingTop: 4 },
  topActions: { flexDirection: 'row', gap: 6 },
  topBtn: { backgroundColor: '#2a2a2a', borderWidth: 1, borderColor: '#555', paddingHorizontal: 10, paddingVertical: 4, borderRadius: 4 },
  topBtnText: { color: '#aaa', fontSize: 12 },
  fps: { color: '#0f0', fontSize: 10, backgroundColor: 'rgba(0,0,0,0.7)', padding: 3 },
  dpad: { position: 'absolute', left: 30, top: '50%', width: 156, height: 156, marginTop: -78, zIndex: 10 },
  dbtn: {
    position: 'absolute', width: 52, height: 52, borderRadius: 26,
    borderWidth: 1, borderColor: '#555',
    justifyContent: 'center', alignItems: 'center',
  },
  dUp:    { top: 0,   left: '50%', marginLeft: -26 },
  dDown:  { bottom: 0, left: '50%', marginLeft: -26 },
  dLeft:  { left: 0,  top: '50%',  marginTop: -26 },
  dRight: { right: 0, top: '50%',  marginTop: -26 },
  dText: { color: '#aaa', fontSize: 20 },
  center: { flex: 1, backgroundColor: '#000', justifyContent: 'center', alignItems: 'center' },
  loadBtn: { backgroundColor: '#2a5a2a', borderWidth: 1, borderColor: '#4a4', paddingHorizontal: 28, paddingVertical: 14 },
  loadText: { color: '#ccc', fontSize: 15 },
  actions: { position: 'absolute', right: 30, top: '50%', gap: 8, marginTop: -64, zIndex: 10 },
  actBtn: {
    width: 60, height: 60, borderRadius: 30,
    borderWidth: 1, borderColor: '#555',
    justifyContent: 'center', alignItems: 'center',
  },
  actText: { color: '#aaa', fontSize: 18 },
  corner: {
    position: 'absolute', bottom: 30, zIndex: 10,
    width: 64, height: 36, borderRadius: 4,
    borderWidth: 1, borderColor: '#555',
    justifyContent: 'center', alignItems: 'center',
  },
  sel: { left: 30 },
  sta: { right: 30 },
  cornerText: { color: '#aaa', fontSize: 13 },
});
