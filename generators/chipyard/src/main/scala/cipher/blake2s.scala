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


// DOC include start: BLAKE2S params
case class BLAKE2SParams(address: BigInt)
// DOC include end: BLAKE2S params


// DOC include start: BLAKE2S key
case object BLAKE2SKey extends Field[Option[BLAKE2SParams]](None)
// DOC include end: BLAKE2S key


class BLAKE2SIO extends Bundle {
	val clk        = Input(Clock())
  val reset      = Input(Bool())
  val cs         = Input(Bool())
  val we         = Input(Bool())
  val address    = Input(UInt(8.W))
  val write_data = Input(UInt(32.W))
  val read_data  = Output(UInt(32.W))
}

// declare register-map structure
object BLAKE2SCtrlRegs {
  val trigger     = 0x00
  val data_a      = 0x04
  val data_b      = 0x08
  val data_c      = 0x0C
}


// DOC include start: BLAKE2S blackbox
class blake2s extends BlackBox with HasBlackBoxResource {
  val io = IO(new BLAKE2SIO)
  addResource("/vsrc/Blake2s/blake2s.v")
  addResource("/vsrc/Blake2s/blake2s_core.v")
  addResource("/vsrc/Blake2s/blake2s_G.v")
  addResource("/vsrc/Blake2s/blake2s_m_select.v")
}
// DOC include end: BLAKE2S blackbox


// DOC include start: BLAKE2S router
class BLAKE2STL(params: BLAKE2SParams, beatBytes: Int)(implicit p: Parameters) extends ClockSinkDomain(ClockSinkParameters())(p) {
  val device = new SimpleDevice("blake2s", Seq("deslab,blake2s"))
  val node = TLRegisterNode(
    address = Seq(AddressSet(params.address, 0xfff)),
    device = device,
    deviceKey =  "reg/control",
    beatBytes = beatBytes)

  override lazy val module = new BLAKE2SImpl
  class BLAKE2SImpl extends Impl {
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
      val impl = Module(new blake2s)
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

// DOC include start: BLAKE2S instance regmap
      node.regmap(
      	BLAKE2SCtrlRegs.trigger -> Seq(
        	RegField(1, cs),
        	RegField(7),
        	RegField(1, we),
        	RegField(7),
        	RegField(1, rst)),
      	BLAKE2SCtrlRegs.data_a -> Seq(RegField(8, address)),
      	BLAKE2SCtrlRegs.data_b -> Seq(RegField(32, write_data)),
      	BLAKE2SCtrlRegs.data_c -> Seq(RegField.r(32, read_data)))
// DOC include end: BLAKE2S instance regmap
    }
  }
}
// DOC include end: BLAKE2S router


// DOC include start: BLAKE2S lazy trait
trait CanHavePeripheryBLAKE2S { this: BaseSubsystem =>
  private val portName = "blake2s"

  private val pbus = locateTLBusWrapper(PBUS)

  p(BLAKE2SKey) match {
    case Some(params) => {
      val blake2s = {
        val blake2s = LazyModule(new BLAKE2STL(params, pbus.beatBytes)(p))
        blake2s.clockNode := pbus.fixedClockNode
        pbus.coupleTo(portName) {
          blake2s.node := TLFragmenter(pbus.beatBytes, pbus.blockBytes) := _
        }
        blake2s
      }
    }
    case None => None
  }
}
// DOC include end: BLAKE2S lazy trait


// DOC include start: BLAKE2S config fragment
class WithBLAKE2S(address: BigInt) extends Config((site, here, up) => {
  case BLAKE2SKey => {
    Some(BLAKE2SParams(address = address))
  }
})
// DOC include end: BLAKE2S config fragment