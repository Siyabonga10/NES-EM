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

    Function("tick") {
      val view = currentView
      if (view == null) {
        NesCoreBridge.nativeTick(ByteArray(0))
        return@Function ByteArray(0)
      }
      val bitmap = view.bitmap
      NesCoreBridge.nativeTickRender(ByteArray(0), bitmap)
      view.postInvalidate()
      ByteArray(0)
    }

    Function("getKeys") { NesCoreBridge.nativeGetKeys() }

    Function("setVolume") { volume: Float -> NesCoreBridge.nativeSetVolume(volume) }

    Function("shutdown") { NesCoreBridge.nativeShutdown() }

    View(NesEmView::class) {}
  }
}
