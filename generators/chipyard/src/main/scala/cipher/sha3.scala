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

// DOC include start: SHA3-512 params
case class SHA3Params(address: BigInt)
// DOC include end: SHA3-512 params

// DOC include start: SHA3-512 key
case object SHA3Key extends Field [Option[SHA3Params]](None)
// DOC include end: SHA3-512 key

class SHA3IO extends Bundle {
  val iClk          = Input(Clock())
  val iReset        = Input(Bool())
  val iChipSelect   = Input(Bool())
  val iWrite        = Input(Bool())
  val iRead         = Input(Bool())
  val iAddress      = Input(UInt(8.W))
  val iWriteData    = Input(UInt(32.W))
  val oReadData     = Output(UInt(32.W))
}

// declare register-map structure
object SHA3CtrlRegs {
  val trigger     = 0x00
  val data_a      = 0x04
  val data_b      = 0x08
  val data_c      = 0x0C
}

// DOC include start: SHA3-512 blackbox
class keccak_wrapper extends BlackBox with HasBlackBoxResource {
  val io = IO(new SHA3IO)
  addResource("/vsrc/sha3/keccak_wrapper.v")
  addResource("/vsrc/sha3/f_permutation.v")
  addResource("/vsrc/sha3/keccak.v")
  addResource("/vsrc/sha3/padder1.v")
  addResource("/vsrc/sha3/padder.v")
  addResource("/vsrc/sha3/rconst.v")
  addResource("/vsrc/sha3/round.v")
}
// DOC include end: SHA3-512 blackbox

// DOC include start: SHA3-512 router
class SHA3TL(params: SHA3Params, beatBytes: Int)(implicit p: Parameters) extends ClockSinkDomain(ClockSinkParameters())(p) {
  val device = new SimpleDevice("sha3", Seq("deslab,sha3-512"))
  val node = TLRegisterNode(
    address = Seq(AddressSet(params.address, 0xfff)),
    device = device,
    deviceKey =  "reg/control",
    beatBytes = beatBytes)

  override lazy val module = new SHA3Impl
  class SHA3Impl extends Impl {
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
      val impl = Module(new keccak_wrapper)
      val impl_io = impl.io

      // mapping input
      impl_io.iClk        := clock
      impl_io.iReset      := reset.asBool || rst
      impl_io.iChipSelect := cs
      impl_io.iWrite      := we
      impl_io.iRead       := !we
      impl_io.iAddress    := address
      impl_io.iWriteData  := write_data

      // mapping output
      read_data           := impl_io.oReadData

      // DOC include start: SHA3-512 instance regmap
      node.regmap(
        SHA3CtrlRegs.trigger -> Seq(
          RegField(1, cs),
          RegField(1, we),
          RegField(1, rst)),
        SHA3CtrlRegs.data_a -> Seq(RegField(8, address)),
        SHA3CtrlRegs.data_b -> Seq(RegField(32, write_data)),
        SHA3CtrlRegs.data_c -> Seq(RegField.r(32, read_data)))
      // DOC include end: SHA3-512 instance regmap
    }
  }
}
// DOC include end: SHA3-512 router

// DOC include start: SHA3-512 lazy trait
trait CanHavePeripheralSHA3 { this: BaseSubsystem =>
  private val portName = "sha3"

  private val pbus = locateTLBusWrapper(PBUS)

  p(SHA3Key) match {
    case Some(params: SHA3Params) => {
      val sha3 = {
        val sha3 = LazyModule(new SHA3TL(params, pbus.beatBytes)(p))
        sha3.clockNode := pbus.fixedClockNode
        pbus.coupleTo(portName) {
          sha3.node := TLFragmenter(pbus.beatBytes, pbus.blockBytes) := _
        }
        sha3
      }
    }
    case None => None
  }
}
// DOC include end: SHA3-512 lazy trait

// DOC include start: SHA3-512 config fragment
class WithSHA3(address: BigInt) extends Config((site, here, up) => {
  case SHA3Key => {
    Some(SHA3Params(address = address))
  }
})
// DOC include end: SHA3-512 config fragment