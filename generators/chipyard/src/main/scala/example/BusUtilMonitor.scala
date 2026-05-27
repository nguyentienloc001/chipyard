package chipyard.example

import chisel3._
import chisel3.util._
import org.chipsalliance.cde.config.{Field, Parameters}
import freechips.rocketchip.diplomacy._
import freechips.rocketchip.regmapper._
import freechips.rocketchip.tilelink._
import freechips.rocketchip.subsystem._

case class BusUtilMonitorParams(
    address: BigInt = 0x10011000L,
    nPorts: Int = 4
)

case object BusUtilMonitorKey extends Field[Option[BusUtilMonitorParams]](None)

class BusUtilMonitorIO(nPorts: Int) extends Bundle {
  val busy = Input(Vec(nPorts, Bool()))
}

class BusUtilMonitor(params: BusUtilMonitorParams, beatBytes: Int)(implicit p: Parameters)
    extends RegisterRouter(
      RegisterRouterParams(
        name = "bus-util-monitor",
        compat = Seq("noc-research,bus-util-monitor"),
        base = params.address,
        size = 0x1000,
        beatBytes = beatBytes
      )
    ) {
  override lazy val module = new LazyModuleImp(this) {
    val io = IO(new BusUtilMonitorIO(params.nPorts))

    val counters = RegInit(VecInit(Seq.fill(params.nPorts)(0.U(64.W))))
    val resetPulse = WireDefault(false.B)

    for (i <- 0 until params.nPorts) {
      when (resetPulse) {
        counters(i) := 0.U
      } .elsewhen (io.busy(i)) {
        counters(i) := counters(i) + 1.U
      }
    }

    val counterFields = (0 until params.nPorts).map { i =>
      (0x00 + i * 8) -> Seq(RegField.r(64, counters(i),
        RegFieldDesc(s"counter_$i", s"active-cycle counter for hart $i")))
    }
    val resetField = 0x20 -> Seq(RegField.w(1, resetPulse,
      RegFieldDesc("reset", "write 1 to clear all counters")))

    regmap((counterFields :+ resetField): _*)
  }
}
