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

    Function("tick") { keys: ByteArray ->
      val view = currentView
      if (view == null) return@Function NesCoreBridge.nativeTick(keys)
      val bitmap = view.bitmap
      val cpuState = NesCoreBridge.nativeTickRender(keys, bitmap)
      view.postInvalidate()
      cpuState
    }

    Function("shutdown") { NesCoreBridge.nativeShutdown() }

    View(NesEmView::class) {}
  }
}
