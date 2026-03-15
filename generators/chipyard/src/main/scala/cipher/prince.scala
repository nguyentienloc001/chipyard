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

// DOC include start: Prince params
case class PrinceParams(address: BigInt)
// DOC include end: Prince params

// DOC include start: Prince key
case object PrinceKey extends Field[Option[PrinceParams]](None)
// DOC include end: Prince key

class PrinceIO extends Bundle {
  val clk 		    = Input(Clock())
  val reset 		  = Input(Bool())
  val cs 		      = Input(Bool())
  val we          = Input(Bool())
  val address     = Input(UInt(8.W))
  val write_data  = Input(UInt(32.W))
  val read_data   = Output(UInt(32.W))
}

// declare register-map structure
object PrinceCtrlRegs {
  val trigger     = 0x00
  val data_a      = 0x04
  val data_b      = 0x08
  val data_c      = 0x0C
}

// DOC include start: Prince blackbox
class prince extends BlackBox with HasBlackBoxResource {
  val io = IO(new PrinceIO)
  addResource("/vsrc/prince/prince.v")
  addResource("/vsrc/prince/prince_core.v")
}
// DOC include end: Prince blackbox

// DOC include start: Prince router
class PrinceTL(params: PrinceParams, beatBytes: Int)(implicit p: Parameters) extends ClockSinkDomain(ClockSinkParameters())(p) {
  val device = new SimpleDevice("prince", Seq("deslab,prince"))
  val node = TLRegisterNode(
    address = Seq(AddressSet(params.address, 0xfff)),
    device = device,
    deviceKey = "reg/control",
    beatBytes = beatBytes)

  override lazy val module = new PrinceImpl
  class PrinceImpl extends Impl {
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
      val impl = Module(new prince)
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

// DOC include start: Prince instance regmap
      node.regmap(
        ChaChaCtrlRegs.trigger -> Seq(
          RegField(1, cs),
          RegField(1, we),
          RegField(1, rst)),
        ChaChaCtrlRegs.data_a -> Seq(RegField(8, address)),
        ChaChaCtrlRegs.data_b -> Seq(RegField(32, write_data)),
        ChaChaCtrlRegs.data_c -> Seq(RegField.r(32, read_data)))
// DOC include end: Prince instance regmap
    }
  }
}
// DOC include end: Prince router

// DOC include start: Prince lazy trait
trait CanHavePeripheryPrince { this: BaseSubsystem =>
  private val portName = "prince"

  private val pbus = locateTLBusWrapper(PBUS)

  p(PrinceKey) match {
    case Some(params) => {
      val prince = LazyModule(new PrinceTL(params, pbus.beatBytes)(p))
      prince.clockNode := pbus.fixedClockNode
      pbus.coupleTo(portName) {
        prince.node := TLFragmenter(pbus.beatBytes, pbus.blockBytes) := _
      }
    }
    case None => None
  }
}
// DOC include end: Prince lazy trait

// DOC include start: Prince config fragment
class WithPrince(address: BigInt) extends Config((site, up, here) => {
  case PrinceKey => {
    Some(PrinceParams(address = address))
  }
})
// DOC include end: Prince config fragment