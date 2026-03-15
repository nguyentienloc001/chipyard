package chipyard.cipher

import chisel3._
import chisel3.util._
import chisel3.experimental.{IntParam, BaseModule}
import freechips.rocketchip.prci._
import freechips.rocketchip.subsystem.{BaseSubsystem, PBUS}
import org.chipsalliance.cde.config.{Parameters, Field, Config}
import freechips.rocketchip.diplomacy._
import freechips.rocketchip.regmapper.{HasRegMap, RegField}
import freechips.rocketchip.tilelink._

// DOC include start: POLY1305 params
case class POLY1305Params(address: BigInt)
// DOC include end: POLY1305 params

// DOC include start: POLY1305 key
case object POLY1305Key extends Field [Option[POLY1305Params]](None)
// DOC include end: POLY1305 key

class POLY1305IO extends Bundle {
  val clk           = Input(Clock())
  val reset         = Input(Bool())
  val cs            = Input(Bool())
  val we            = Input(Bool())
  val address       = Input(UInt(8.W))
  val write_data    = Input(UInt(32.W))
  val read_data     = Output(UInt(32.W))
}

// declare register-map structure
object POLY1305CtrlRegs {
  val trigger     = 0x00
  val data_a      = 0x04
  val data_b      = 0x08
  val data_c      = 0x0C
}

// DOC include start: POLY1305 blackbox
class poly1305 extends BlackBox with HasBlackBoxResource {
  val io = IO(new POLY1305IO)
  addResource("/vsrc/poly1305/poly1305.v")
  addResource("/vsrc/poly1305/poly1305_core.v")
  addResource("/vsrc/poly1305/poly1305_final.v")
  addResource("/vsrc/poly1305/poly1305_mulacc.v")
  addResource("/vsrc/poly1305/poly1305_pblock.v")
}
// DOC include end: POLY1305 blackbox

// DOC include start: POLY1305 router
class POLY1305TL(params: POLY1305Params, beatBytes: Int)(implicit p: Parameters) extends ClockSinkDomain(ClockSinkParameters())(p) {
  val device = new SimpleDevice("poly1305", Seq("deslab,poly1305"))
  val node = TLRegisterNode(
    address = Seq(AddressSet(params.address, 0xfff)),
    device = device,
    deviceKey =  "reg/control",
    beatBytes = beatBytes)

  override lazy val module = new POLY1305Impl
  class POLY1305Impl extends Impl {
    withClockAndReset(clock, reset) {
      // declare input
      val address     = Reg(UInt(8.W))
      val write_data  = Reg(UInt(32.W))
      val rst         = RegInit(false.B)
      val cs          = RegInit(false.B)
      val we          = RegInit(false.B)

      // declare output
      val read_data   = Wire(UInt(32.W))

      // HW instantiation
      val impl = Module(new poly1305)
      val impl_io = impl.io

      // mapping input
      impl_io.clk         := clock
      impl_io.reset       := reset.asBool || rst
      impl_io.cs          := cs
      impl_io.we          := we
      impl_io.address     := address
      impl_io.write_data  := write_data

      // mapping output
      read_data           := impl_io.read_data

      // DOC include start: POLY1305 instance regmap
      node.regmap(
        POLY1305CtrlRegs.trigger -> Seq(
          RegField(1, cs),
          RegField(1, we),
          RegField(1, rst)),
        POLY1305CtrlRegs.data_a -> Seq(RegField(8, address)),
        POLY1305CtrlRegs.data_b -> Seq(RegField(32, write_data)),
        POLY1305CtrlRegs.data_c -> Seq(RegField.r(32, read_data)))
      // DOC include end: POLY1305 instance regmap
    }
  }
}
// DOC include end: POLY1305 router

// DOC include start: POLY1305 lazy trait
trait CanHavePeripheralPOLY1305 { this: BaseSubsystem =>
  private val portName = "poly1305"

  private val pbus = locateTLBusWrapper(PBUS)

  p(POLY1305Key) match {
    case Some(params: POLY1305Params) => {
      val poly1305 = {
        val poly1305 = LazyModule(new POLY1305TL(params, pbus.beatBytes)(p))
        poly1305.clockNode := pbus.fixedClockNode
        pbus.coupleTo(portName) {
          poly1305.node := TLFragmenter(pbus.beatBytes, pbus.blockBytes) := _
        }
        poly1305
      }
    }
    case None => None
  }
}
// DOC include end: POLY1305 lazy trait

// DOC include start: POLY1305 config fragment
class WithPOLY1305(address: BigInt) extends Config((site, here, up) => {
  case POLY1305Key => {
    Some(POLY1305Params(address = address))
  }
})
// DOC include end: POLY1305 config fragment