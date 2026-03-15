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

// DOC include start: Present params
case class PresentParams(address: BigInt)
// DOC include end: Present params

// DOC include start: Present key
case object PresentKey extends Field[Option[PresentParams]](None)
// DOC include end: Present key

class PresentIO extends Bundle {
  val clk         = Input(Clock())
  val iReset      = Input(Bool())
  val iChipselect = Input(Bool())
  val iWriteRead  = Input(Bool())
  val iAddress    = Input(UInt(8.W))
  val idat        = Input(UInt(32.W))
  val odat        = Output(UInt(32.W))
}

// declare register-map structure
object PresentCtrlRegs {
  val trigger     = 0x00
  val data_a      = 0x04
  val data_b      = 0x08
  val data_c      = 0x0C
}

// DOC include start: Present blackbox
class present_wrapper extends BlackBox with HasBlackBoxResource {
  val io = IO(new PresentIO)
  addResource("/vsrc/present_dmpresent/present_wrapper.v")
  addResource("/vsrc/present_dmpresent/present_core.v")
  addResource("/vsrc/present_dmpresent/present_encrypt.v")
  addResource("/vsrc/present_dmpresent/sbox.v")
  addResource("/vsrc/present_dmpresent/pbox.v")
  addResource("/vsrc/present_dmpresent/present_decrypt.v")
  addResource("/vsrc/present_dmpresent/inv_sbox.v")
  addResource("/vsrc/present_dmpresent/inv_pbox.v")
}
// DOC include end: Present blackbox

// DOC include Present router
class PresentTL(params: PresentParams, beatBytes: Int)(implicit p: Parameters) extends ClockSinkDomain(ClockSinkParameters())(p) {
  val device = new SimpleDevice("present", Seq("deslab,present"))
  val node = TLRegisterNode(
    address = Seq(AddressSet(params.address, 0xfff)),
    device = device,
    deviceKey = "reg/control",
    beatBytes = beatBytes)

  override lazy val module = new PresentImpl
  class PresentImpl extends Impl {
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
      val impl = Module(new present_wrapper)
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

      // DOC include start: Present instance regmap
      node.regmap(
        PresentCtrlRegs.trigger -> Seq(
          RegField(1, cs),
          RegField(1, we),
          RegField(1, rst)),
        PresentCtrlRegs.data_a -> Seq(RegField(8, address)),
        PresentCtrlRegs.data_b -> Seq(RegField(32, write_data)),
        PresentCtrlRegs.data_c -> Seq(RegField.r(32, read_data)))
      // DOC include end: Present instance regmap
    }
  }
}
// DOC include Present router

// DOC include start: Present lazy trait
trait CanHavePeripheryPresent { this: BaseSubsystem =>
  private val portName = "present"

  private val pbus = locateTLBusWrapper(PBUS)

  p(PresentKey) match {
    case Some(params) => {
      val present = LazyModule(new PresentTL(params, pbus.beatBytes)(p))
      present.clockNode := pbus.fixedClockNode
      pbus.coupleTo(portName) {
        present.node := TLFragmenter(pbus.beatBytes, pbus.blockBytes) := _
      }
    }
    case None => None
  }
}
// DOC include end: Present lazy trait

// DOC include start: Present config fragment
class WithPresent(address: BigInt) extends Config((site, up, here) => {
  case PresentKey => {
    Some(PresentParams(address = address))
  }
})
// DOC include end: Present config fragment
