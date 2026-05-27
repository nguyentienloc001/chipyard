package chipyard.example

import chisel3._
import org.chipsalliance.cde.config.{Config, Parameters}
import freechips.rocketchip.diplomacy._
import freechips.rocketchip.subsystem._
import freechips.rocketchip.tilelink._

trait CanHaveBusUtilMonitor { this: BaseSubsystem =>
  private val pbus = locateTLBusWrapper(PBUS)
  private val sbus = locateTLBusWrapper(SBUS)

  val busUtilMonitor = p(BusUtilMonitorKey).map { rawParams =>
    val monitor = LazyModule(new BusUtilMonitor(rawParams, pbus.beatBytes))
    monitor.clockNode := pbus.fixedClockNode
    pbus.coupleTo("bus-util-monitor") {
      monitor.node := TLFragmenter(pbus.beatBytes, pbus.blockBytes) := _
    }
    monitor
  }

  InModuleBody {
    busUtilMonitor.foreach { mon =>
      val nPorts = mon.module.io.busy.length
      val inEdges = sbus.inwardNode.in
      val taps = inEdges.take(nPorts)
      // For any inbound edges we don't have ports for, drive busy=false
      for (i <- 0 until nPorts) {
        if (i < taps.length) {
          val (bundle, _) = taps(i)
          mon.module.io.busy(i) := bundle.a.fire
        } else {
          mon.module.io.busy(i) := false.B
        }
      }
    }
  }
}

class WithBusUtilMonitor extends Config((_, _, _) => {
  case BusUtilMonitorKey => Some(BusUtilMonitorParams())
})
