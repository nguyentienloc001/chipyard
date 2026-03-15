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

// DOC include start: AES params
case class AESParams(address: BigInt)
// DOC include end: AES params

// DOC include start: AES key
case object AESKey extends Field [Option[AESParams]](None)
// DOC include end: AES key

class AESIO extends Bundle {
  val clk           = Input(Clock())
  val reset         = Input(Bool())
  val cs            = Input(Bool())
  val we            = Input(Bool())
  val address       = Input(UInt(8.W))
  val write_data    = Input(UInt(32.W))
  val read_data     = Output(UInt(32.W))
}

// declare register-map structure
object AESCtrlRegs {
  val trigger     = 0x00
  val data_a      = 0x04
  val data_b      = 0x08
  val data_c      = 0x0C
}

// DOC include start: AES blackbox
class aes extends BlackBox with HasBlackBoxResource {
  val io = IO(new AESIO)
  addResource("/vsrc/aes/aes.v")
  addResource("/vsrc/aes/aes_core.v")
  addResource("/vsrc/aes/aes_decipher_block.v")
  addResource("/vsrc/aes/aes_encipher_block.v")
  addResource("/vsrc/aes/aes_key_mem.v")
  addResource("/vsrc/aes/aes_inv_sbox.v")
  addResource("/vsrc/aes/aes_sbox.v")
}
// DOC include end: AES blackbox

// DOC include start: AES router
class AESTL(params: AESParams, beatBytes: Int)(implicit p: Parameters) extends ClockSinkDomain(ClockSinkParameters())(p) {
  val device = new SimpleDevice("aes", Seq("deslab,aes"))
  val node = TLRegisterNode(
    address = Seq(AddressSet(params.address, 0xfff)),
    device = device,
    deviceKey =  "reg/control",
    beatBytes = beatBytes)

  override lazy val module = new AESImpl
  class AESImpl extends Impl {
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
      val impl = Module(new aes)
      val impl_io = impl.io

      // mapping input
      impl_io.clk        := clock
      impl_io.reset      := reset.asBool || rst
      impl_io.cs         := cs
      impl_io.we         := we
      impl_io.address    := address
      impl_io.write_data := write_data

      // mapping output
      read_data           := impl_io.read_data

      // DOC include start: AES instance regmap
      node.regmap(
        AESCtrlRegs.trigger -> Seq(
          RegField(1, cs),
          RegField(1, we),
          RegField(1, rst)),
        AESCtrlRegs.data_a -> Seq(RegField(8, address)),
        AESCtrlRegs.data_b -> Seq(RegField(32, write_data)),
        AESCtrlRegs.data_c -> Seq(RegField.r(32, read_data)))
      // DOC include end: AES instance regmap
    }
  }
}
// DOC include end: AES router

// DOC include start: AES lazy trait
trait CanHavePeripheralAES { this: BaseSubsystem =>
  private val portName = "aes"

  private val pbus = locateTLBusWrapper(PBUS)

  p(AESKey) match {
    case Some(params: AESParams) => {
      val aes = {
        val aes = LazyModule(new AESTL(params, pbus.beatBytes)(p))
        aes.clockNode := pbus.fixedClockNode
        pbus.coupleTo(portName) {
          aes.node := TLFragmenter(pbus.beatBytes, pbus.blockBytes) := _
        }
        aes
      }
    }
    case None => None
  }
}
// DOC include end: AES lazy trait

// DOC include start: AES config fragment
class WithAES(address: BigInt) extends Config((site, here, up) => {
  case AESKey => {
    Some(AESParams(address = address))
  }
})
// DOC include end: AES config fragment