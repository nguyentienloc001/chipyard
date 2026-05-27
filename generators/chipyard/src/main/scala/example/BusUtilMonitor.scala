package chipyard.example

import chisel3._
import chisel3.util._
import org.chipsalliance.cde.config.{Field, Parameters}
import freechips.rocketchip.diplomacy._
import freechips.rocketchip.prci._
import freechips.rocketchip.regmapper._
import freechips.rocketchip.tilelink._

case class BusUtilMonitorParams(
    address: BigInt = 0x10011000L,
    nPorts: Int = 4
)

case object BusUtilMonitorKey extends Field[Option[BusUtilMonitorParams]](None)

class BusUtilMonitorIO(val nPorts: Int) extends Bundle {
  val busy = Input(Vec(nPorts, Bool()))
}

trait HasBusUtilMonitorIO {
  def io: BusUtilMonitorIO
}

class BusUtilMonitor(params: BusUtilMonitorParams, beatBytes: Int)(implicit p: Parameters)
    extends ClockSinkDomain(ClockSinkParameters())(p) {

  val device = new SimpleDevice("bus-util-monitor", Seq("noc-research,bus-util-monitor"))
  val node = TLRegisterNode(
    Seq(AddressSet(params.address, 0x1000 - 1)),
    device,
    "reg/control",
    beatBytes = beatBytes
  )

  override lazy val module = new Impl
  class Impl extends LazyModuleImp(this) with HasBusUtilMonitorIO {
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

    node.regmap((counterFields :+ resetField): _*)
  }
}
