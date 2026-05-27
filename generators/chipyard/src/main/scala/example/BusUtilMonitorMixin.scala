package chipyard.example

import chisel3._
import org.chipsalliance.cde.config.{Config, Parameters}
import freechips.rocketchip.diplomacy._
import freechips.rocketchip.subsystem._
import freechips.rocketchip.tile._
import freechips.rocketchip.tilelink._

trait CanHaveBusUtilMonitor { this: BaseSubsystem =>
  private val pbus = locateTLBusWrapper(PBUS)

  val busUtilMonitor = p(BusUtilMonitorKey).map { rawParams =>
    val nTiles = totalTiles.size
    val params = rawParams.copy(nPorts = nTiles)
    val monitor = LazyModule(new BusUtilMonitor(params, pbus.beatBytes))
    pbus.coupleTo("bus-util-monitor") {
      monitor.node := TLFragmenter(pbus) := _
    }
    monitor
  }

  InModuleBody {
    busUtilMonitor.foreach { mon =>
      val tileList = totalTiles.values.toSeq
      require(tileList.length == mon.module.io.busy.length,
        s"BusUtilMonitor: ${tileList.length} tiles vs ${mon.module.io.busy.length} ports")
      tileList.zipWithIndex.foreach { case (tile, i) =>
        val masterEdges = tile.tlMasterXbar.node.edges.out
        require(masterEdges.nonEmpty, s"Tile $i has no outbound TL master edges")
        val fires = tile.tlMasterXbar.node.out.map { case (bundle, _) => bundle.a.fire }
        mon.module.io.busy(i) := fires.reduce(_ || _)
      }
    }
  }
}

class WithBusUtilMonitor extends Config((_, _, _) => {
  case BusUtilMonitorKey => Some(BusUtilMonitorParams())
})
