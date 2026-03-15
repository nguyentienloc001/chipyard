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

// DOC include start: ChaCha params
case class ChaChaParams(address: BigInt)
// DOC include end: ChaCha params

// DOC include start: ChaCha key
case object ChaChaKey extends Field[Option[ChaChaParams]](None)
// DOC include end: ChaCha key

class ChaChaIO extends Bundle {
  val clk 		    = Input(Clock())
  val reset 		  = Input(Bool())
  val cs 		      = Input(Bool())
  val we          = Input(Bool())
  val addr        = Input(UInt(8.W))
  val write_data  = Input(UInt(32.W))
  val read_data   = Output(UInt(32.W))
}

// declare register-map structure
object ChaChaCtrlRegs {
  val trigger     = 0x00
  val data_a      = 0x04
  val data_b      = 0x08
  val data_c      = 0x0C
}

// DOC include start: ChaCha blackbox
class chacha extends BlackBox with HasBlackBoxResource {
  val io = IO(new ChaChaIO)
  addResource("/vsrc/chacha/chacha.v")
  addResource("/vsrc/chacha/chacha_core.v")
  addResource("/vsrc/chacha/chacha_qr.v")
}
// DOC include end: ChaCha blackbox

// DOC include ChaCha router
class ChaChaTL(params: ChaChaParams, beatBytes: Int)(implicit p: Parameters) extends ClockSinkDomain(ClockSinkParameters())(p) {
  val device = new SimpleDevice("chacha", Seq("deslab,chacha"))
  val node = TLRegisterNode(
    address = Seq(AddressSet(params.address, 0xfff)),
    device = device,
    deviceKey = "reg/control",
    beatBytes = beatBytes)

  override lazy val module = new ChaChaImpl
  class ChaChaImpl extends Impl {
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
      val impl = Module(new chacha)
      val impl_io = impl.io

      // mapping input
      impl_io.clk         := clock
      impl_io.reset       := reset.asBool || rst
      impl_io.cs          := cs
      impl_io.we          := we
      impl_io.addr        := address
      impl_io.write_data  := write_data

      // mapping output
      read_data           := impl_io.read_data

// DOC include start: ChaCha instance regmap
      node.regmap(
        ChaChaCtrlRegs.trigger -> Seq(
          RegField(1, cs),
          RegField(1, we),
          RegField(1, rst)),
        ChaChaCtrlRegs.data_a -> Seq(RegField(8, address)),
        ChaChaCtrlRegs.data_b -> Seq(RegField(32, write_data)),
        ChaChaCtrlRegs.data_c -> Seq(RegField.r(32, read_data)))
// DOC include end: ChaCha instance regmap
    }
  }
}
// DOC include ChaCha router

// DOC include start: ChaCha lazy trait
trait CanHavePeripheryChaCha { this: BaseSubsystem =>
  private val portName = "chacha"

  private val pbus = locateTLBusWrapper(PBUS)

  p(ChaChaKey) match {
    case Some(params) => {
      val chacha = LazyModule(new ChaChaTL(params, pbus.beatBytes)(p))
      chacha.clockNode := pbus.fixedClockNode
      pbus.coupleTo(portName) {
        chacha.node := TLFragmenter(pbus.beatBytes, pbus.blockBytes) := _
      }
    }
    case None => None
  }
}
// DOC include end: ChaCha lazy trait

// DOC include start: ChaCha config fragment
class WithChaCha(address: BigInt) extends Config((site, up, here) => {
  case ChaChaKey => {
    Some(ChaChaParams(address = address))
  }
})
// DOC include end: ChaCha config fragment