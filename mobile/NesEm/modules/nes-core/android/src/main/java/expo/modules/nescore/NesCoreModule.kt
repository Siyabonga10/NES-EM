package expo.modules.nescore

import expo.modules.kotlin.modules.Module
import expo.modules.kotlin.modules.ModuleDefinition

class NesCoreModule : Module() {
  companion object {
    @JvmStatic
    var currentView: NesEmView? = null
  }

  override fun definition() = ModuleDefinition {
    Name("NesCore")

    AsyncFunction("hello") {
      try {
        System.loadLibrary("nescore")
        "loaded"
      } catch (e: Throwable) {
        throw Exception("Failed: ${e.message}")
      }
    }

    Function("loadRom") { rom: ByteArray ->
      val rc = NesCoreBridge.nativeLoadRom(rom)
      if (rc != 0) throw Exception("ROM load failed: $rc")
    }

    Function("startLoop") {
      val view = currentView
      if (view != null) NesCoreBridge.nativeStartLoop(view.bitmap, view)
    }

    Function("stopLoop") {
      NesCoreBridge.nativeStopLoop()
    }

    Function("pauseLoop") {
      NesCoreBridge.nativePauseLoop()
    }

    Function("resumeLoop") {
      NesCoreBridge.nativeResumeLoop()
    }

    Function("setUncapped") { uncapped: Boolean -> NesCoreBridge.nativeSetUncapped(uncapped) }

    Function("getFps") { NesCoreBridge.nativeGetFps() }

    Function("getKeys") { NesCoreBridge.nativeGetKeys() }

    Function("setVolume") { volume: Float -> NesCoreBridge.nativeSetVolume(volume) }

    Function("shutdown") { NesCoreBridge.nativeShutdown() }

    View(NesEmView::class) {}
  }
}
