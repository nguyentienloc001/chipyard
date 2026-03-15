package chipyard.cipher

import chisel3._
import chisel3.util._
import freechips.rocketchip.diplomacy.{AddressSet, LazyModule}
import freechips.rocketchip.prci.{ClockSinkDomain, ClockSinkParameters}
import freechips.rocketchip.regmapper.RegField
import freechips.rocketchip.resources.SimpleDevice
import freechips.rocketchip.subsystem.{BaseSubsystem, PBUS}
import freechips.rocketchip.tilelink.{TLFragmenter, TLRegisterNode}
import org.chipsalliance.cde.config.{Config, Field, Parameters}

// DOC include start: MyTimer params
case class MyTimerParams(address: BigInt)
// DOC include end: MyTimer params

// DOC include start: MyTimer key
case object MyTimerKey extends Field[Option[MyTimerParams]](None)
// DOC include end: MyTimer key

class MyTimerIO extends Bundle {
  val clk 	  = Input(Clock())
  val reset 	= Input(Bool())
  val clear 	= Input(Bool())
  val start   = Input(Bool())
  val pause   = Input(Bool())
  val count   = Output(UInt(32.W))
}

// declare register-map structure
object MyTimerCtrlRegs {
  val trigger     = 0x00
  val data        = 0x04
}

// DOC include start: MyTimer blackbox
class mytimer extends BlackBox with HasBlackBoxResource {
  val io = IO(new MyTimerIO)
  addResource("/vsrc/mytimer/mytimer.v")
}
// DOC include end: MyTimer blackbox

// DOC include MyTimer router
class MyTimerTL(params: MyTimerParams, beatBytes: Int)(implicit p: Parameters) extends ClockSinkDomain(ClockSinkParameters())(p) {
  val device = new SimpleDevice("mytimer", Seq("deslab,mytimer"))
  val node = TLRegisterNode(
    address = Seq(AddressSet(params.address, 0xfff)),
    device = device,
    deviceKey = "reg/control",
    beatBytes = beatBytes)

  override lazy val module = new MyTimerImpl
  class MyTimerImpl extends Impl {
    withClockAndReset(clock, reset) {
      // declare input
      val reset   = RegInit(false.B)
      val clear   = RegInit(false.B)
      val start   = RegInit(false.B)
      val pause   = RegInit(false.B)

      // declare output
      val count   = Wire(UInt(32.W))

      // HW instantiation
      val impl = Module(new mytimer)
      val impl_io = impl.io

      // mapping input
      impl_io.clk         := clock
      impl_io.reset       := reset.asBool || reset
      impl_io.clear       := clear
      impl_io.start       := start
      impl_io.pause       := pause

      // mapping output
      count           := impl_io.count

      // DOC include start: MyTimer instance regmap
      node.regmap(
        MyTimerCtrlRegs.trigger -> Seq(
          RegField(1, start),
          RegField(1, pause),
          RegField(1, clear),
          RegField(1, reset)),
        MyTimerCtrlRegs.data -> Seq(RegField.r(32, count)))
      // DOC include end: MyTimer instance regmap
    }
  }
}
// DOC include MyTimer router

// DOC include start: MyTimer lazy trait
trait CanHavePeripheryMyTimer { this: BaseSubsystem =>
  private val portName = "mytimer"

  private val pbus = locateTLBusWrapper(PBUS)

  p(MyTimerKey) match {
    case Some(params) => {
      val mytimer = LazyModule(new MyTimerTL(params, pbus.beatBytes)(p))
      mytimer.clockNode := pbus.fixedClockNode
      pbus.coupleTo(portName) {
        mytimer.node := TLFragmenter(pbus.beatBytes, pbus.blockBytes) := _
      }
    }
    case None => None
  }
}
// DOC include end: MyTimer lazy trait

// DOC include start: MyTimer config fragment
class WithMyTimer(address: BigInt) extends Config((site, up, here) => {
  case MyTimerKey => {
    Some(MyTimerParams(address = address))
  }
})
// DOC include end: MyTimer config fragment