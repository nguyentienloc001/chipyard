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

// DOC include start: DMPresent params
case class DMPresentParams(address: BigInt)
// DOC include end: DMPresent params

// DOC include start: DMPresent key
case object DMPresentKey extends Field[Option[DMPresentParams]](None)
// DOC include end: DMPresent key

class DMPresentIO extends Bundle {
  val clk         = Input(Clock())
  val iReset      = Input(Bool())
  val iChipselect = Input(Bool())
  val iWriteRead  = Input(Bool())
  val iAddress    = Input(UInt(8.W))
  val idat        = Input(UInt(32.W))
  val odat        = Output(UInt(32.W))
}

// declare register-map structure
object DMPresentCtrlRegs {
  val trigger     = 0x00
  val data_a      = 0x04
  val data_b      = 0x08
  val data_c      = 0x0C
}

// DOC include start: DMPresent blackbox
class dmpresent_wrapper extends BlackBox with HasBlackBoxResource {
  val io = IO(new DMPresentIO)
  addResource("/vsrc/present_dmpresent/dmpresent_wrapper.v")
  addResource("/vsrc/present_dmpresent/dmpresent.v")
  addResource("/vsrc/present_dmpresent/sbox.v")
  addResource("/vsrc/present_dmpresent/pbox.v")
}
// DOC include end: DMPresent blackbox

// DOC include DMPresent router
class DMPresentTL(params: DMPresentParams, beatBytes: Int)(implicit p: Parameters) extends ClockSinkDomain(ClockSinkParameters())(p) {
  val device = new SimpleDevice("dmpresent", Seq("deslab,dmpresent"))
  val node = TLRegisterNode(
    address = Seq(AddressSet(params.address, 0xfff)),
    device = device,
    deviceKey = "reg/control",
    beatBytes = beatBytes)

  override lazy val module = new DMPresentImpl
  class DMPresentImpl extends Impl {
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
      val impl = Module(new dmpresent_wrapper)
      val impl_io = impl.io

      // mapping input
      impl_io.clk           := clock
      impl_io.iReset        := reset.asBool || rst
      impl_io.iChipselect   := cs
      impl_io.iWriteRead    := we
      impl_io.iAddress      := address
      impl_io.idat          := write_data

      // mapping output
      read_data           := impl_io.odat

      // DOC include start: DMPresent instance regmap
      node.regmap(
        DMPresentCtrlRegs.trigger -> Seq(
          RegField(1, cs),
          RegField(1, we),
          RegField(1, rst)),
        DMPresentCtrlRegs.data_a -> Seq(RegField(8, address)),
        DMPresentCtrlRegs.data_b -> Seq(RegField(32, write_data)),
        DMPresentCtrlRegs.data_c -> Seq(RegField.r(32, read_data)))
      // DOC include end: DMPresent instance regmap
    }
  }
}
// DOC include DMPresent router

// DOC include start: DMPresent lazy trait
trait CanHavePeripheryDMPresent { this: BaseSubsystem =>
  private val portName = "dmpresent"

  private val pbus = locateTLBusWrapper(PBUS)

  p(DMPresentKey) match {
    case Some(params) => {
      val dmpresent = LazyModule(new DMPresentTL(params, pbus.beatBytes)(p))
      dmpresent.clockNode := pbus.fixedClockNode
      pbus.coupleTo(portName) {
        dmpresent.node := TLFragmenter(pbus.beatBytes, pbus.blockBytes) := _
      }
    }
    case None => None
  }
}
// DOC include end: DMPresent lazy trait

// DOC include start: DMPresent config fragment
class WithDMPresent(address: BigInt) extends Config((site, up, here) => {
  case DMPresentKey => {
    Some(DMPresentParams(address = address))
  }
})
// DOC include end: DMPresent config fragment