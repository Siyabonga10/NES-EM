import { Stack } from 'expo-router';
import { StatusBar } from 'expo-status-bar';
import { useEffect } from 'react';
import * as SystemUI from 'expo-system-ui';

export default function RootLayout() {
  useEffect(() => {
    SystemUI.setBackgroundColorAsync('#111');
  }, []);

  return (
    <>
      <StatusBar hidden />
      <Stack screenOptions={{ headerShown: false, animation: 'none' }} />
    </>
  );
}
