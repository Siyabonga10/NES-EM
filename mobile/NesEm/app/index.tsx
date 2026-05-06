import { View, Text, Pressable, StyleSheet, Alert, AppState } from 'react-native';
import { useEffect, useState, useRef, useCallback } from 'react';
import * as DocumentPicker from 'expo-document-picker';
import * as FileSystem from 'expo-file-system/legacy';
import { hello, loadRom, tick, shutdown, NesEmView } from '@/modules/nes-core';

const KEYS: Record<string, number> = {
  a: 0, b: 1, up: 2, down: 3, left: 4, right: 5, start: 6, select: 7,
};
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
  const keysRef = useRef([0,0,0,0,0,0,0,0]);
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
      tick(keysRef.current);
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

  const press = useCallback((key: string) => {
    const idx = KEYS[key]; if (idx !== undefined) keysRef.current[idx] = 1;
    setPressedKeys(prev => new Set(prev).add(key));
  }, []);
  const release = useCallback((key: string) => {
    const idx = KEYS[key]; if (idx !== undefined) keysRef.current[idx] = 0;
    setPressedKeys(prev => { const next = new Set(prev); next.delete(key); return next; });
  }, []);

  const btnColor = (key: string) => pressedKeys.has(key) ? '#4a4a4a' : '#2a2a2a';

  return (
    <View style={styles.root}>
      {/* D-pad — LEFT */}
      <View style={styles.dpad}>
        <Pressable style={[styles.dbtn, styles.dUp, { backgroundColor: btnColor('up') }]} onPressIn={() => press('up')} onPressOut={() => release('up')}>
          <Text style={styles.dText}>▲</Text>
        </Pressable>
        <Pressable style={[styles.dbtn, styles.dDown, { backgroundColor: btnColor('down') }]} onPressIn={() => press('down')} onPressOut={() => release('down')}>
          <Text style={styles.dText}>▼</Text>
        </Pressable>
        <Pressable style={[styles.dbtn, styles.dLeft, { backgroundColor: btnColor('left') }]} onPressIn={() => press('left')} onPressOut={() => release('left')}>
          <Text style={styles.dText}>◄</Text>
        </Pressable>
        <Pressable style={[styles.dbtn, styles.dRight, { backgroundColor: btnColor('right') }]} onPressIn={() => press('right')} onPressOut={() => release('right')}>
          <Text style={styles.dText}>►</Text>
        </Pressable>
      </View>

      {/* A/B — RIGHT */}
      <View style={styles.actions}>
        <Pressable style={[styles.actBtn, { backgroundColor: btnColor('b') }]} onPressIn={() => press('b')} onPressOut={() => release('b')}>
          <Text style={styles.actText}>B</Text>
        </Pressable>
        <Pressable style={[styles.actBtn, { backgroundColor: btnColor('a') }]} onPressIn={() => press('a')} onPressOut={() => release('a')}>
          <Text style={styles.actText}>A</Text>
        </Pressable>
      </View>

      {/* Game view */}
      <View style={styles.center}>
        {!romLoaded ? (
          <Pressable style={styles.loadBtn} onPress={handleLoadRom}>
            <Text style={styles.loadText}>Load ROM</Text>
          </Pressable>
        ) : (
          <NesEmView style={styles.gameView} />
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
      <Pressable style={[styles.corner, styles.sel, { backgroundColor: btnColor('select') }]} onPressIn={() => press('select')} onPressOut={() => release('select')}>
        <Text style={styles.cornerText}>Sel</Text>
      </Pressable>
      <Pressable style={[styles.corner, styles.sta, { backgroundColor: btnColor('start') }]} onPressIn={() => press('start')} onPressOut={() => release('start')}>
        <Text style={styles.cornerText}>Start</Text>
      </Pressable>
    </View>
  );
}

const styles = StyleSheet.create({
  root: { flex: 1, backgroundColor: '#111' },
  topBar: { position: 'absolute', top: 0, left: 0, right: 0, flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', zIndex: 10, paddingHorizontal: 8, paddingTop: 4 },
  topActions: { flexDirection: 'row', gap: 6 },
  topBtn: { backgroundColor: '#2a2a2a', borderWidth: 1, borderColor: '#555', paddingHorizontal: 10, paddingVertical: 4, borderRadius: 4 },
  topBtnText: { color: '#aaa', fontSize: 12 },
  fps: { color: '#0f0', fontSize: 10, backgroundColor: 'rgba(0,0,0,0.7)', padding: 3 },
  dpad: { position: 'absolute', left: 30, top: '50%', width: 156, height: 156, marginTop: -78, zIndex: 1 },
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
  gameView: { flex: 1, width: '100%', height: '100%' },
  loadBtn: { backgroundColor: '#2a5a2a', borderWidth: 1, borderColor: '#4a4', paddingHorizontal: 28, paddingVertical: 14 },
  loadText: { color: '#ccc', fontSize: 15 },
  actions: { position: 'absolute', right: 30, top: '50%', gap: 8, marginTop: -64, zIndex: 1 },
  actBtn: {
    width: 60, height: 60, borderRadius: 30,
    borderWidth: 1, borderColor: '#555',
    justifyContent: 'center', alignItems: 'center',
  },
  actText: { color: '#aaa', fontSize: 18 },
  corner: {
    position: 'absolute', bottom: 30, zIndex: 1,
    width: 64, height: 36, borderRadius: 4,
    borderWidth: 1, borderColor: '#555',
    justifyContent: 'center', alignItems: 'center',
  },
  sel: { left: 30 },
  sta: { right: 30 },
  cornerText: { color: '#aaa', fontSize: 13 },
});
