package chipyard.cipher

//import sys.process._

import chisel3._
import chisel3.util._
import chisel3.experimental.{IntParam, BaseModule}
import freechips.rocketchip.prci._
import freechips.rocketchip.subsystem.{BaseSubsystem, PBUS}
import org.chipsalliance.cde.config.{Parameters, Field, Config}
import freechips.rocketchip.diplomacy._
import freechips.rocketchip.regmapper.{HasRegMap, RegField}
import freechips.rocketchip.tilelink._
//import freechips.rocketchip.util.UIntIsOneOf

// DOC include start: ROM params
case class ROMParams(address: BigInt)
// DOC include end: ROM params

// DOC include start: ROM key
case object ROMKey extends Field[Option[ROMParams]](None)
// DOC include end: ROM key

class ROMIO extends Bundle {
  val clk		= Input(Clock())
  val addr 	= Input(UInt(3.W))
  val dout 	= Output(UInt(32.W))
}

//class ROMTopIO extends Bundle {
//  val rom_busy = Output(Bool())
//}
//
//trait HasROMTopIO {
//  def io: ROMTopIO
//}

// DOC include start: ROM blackbox
class rom_8x32 extends BlackBox with HasBlackBoxResource {
  val io = IO(new ROMIO)
  addResource("/vsrc/rom_8x32.v")
}
// DOC include end: ROM blackbox

// DOC include start: ROM router
class ROMTL(params: ROMParams, beatBytes: Int)(implicit p: Parameters) extends ClockSinkDomain(ClockSinkParameters())(p) {
  val device = new SimpleDevice("rom_8x32", Seq("custom,rom_8x32")) 
  val node = TLRegisterNode(Seq(AddressSet(params.address, 0xfff)), device, "reg/control", beatBytes=beatBytes)

  override lazy val module = new ROMImpl
  class ROMImpl extends Impl {
  	//val io = IO(new ROMTopIO)
  	withClock(clock) {
  		// declare input
  		val addr = Reg(UInt(3.W))
  		// declare output
  		val dout = Wire(UInt(32.W))

  		// HW instantiation
      val impl = Module(new rom_8x32)
      val impl_io = impl.io

  		// mapping input
  		impl_io.clk 	:= clock
  		impl_io.addr 	:= addr
  		// mapping output
  		dout 					:= impl_io.dout

  		//io.rom_busy		:= false.B

// DOC include start: ROM instance regmap
      node.regmap(
				0x00 -> Seq(
					RegField(3, addr)),
				0x04 -> Seq(
					RegField.r(32, dout)))
// DOC include end: ROM instance regmap
    }
  }
}
// DOC include end: ROM router

// DOC include start: ROM lazy trait
trait CanHavePeripheryROM { this: BaseSubsystem =>
  private val portName = "rom"

  private val pbus = locateTLBusWrapper(PBUS)

  // Only build if we are using the TL (nonAXI4) version
  val rom_busy = p(ROMKey) match {
    case Some(params) => {
      val rom = {
        val rom = LazyModule(new ROMTL(params, pbus.beatBytes)(p))
        rom.clockNode := pbus.fixedClockNode
        pbus.coupleTo(portName) {
          rom.node := TLFragmenter(pbus.beatBytes, pbus.blockBytes) := _
        }
        rom
      }

//      val rom_busy = InModuleBody {
//        val busy = IO(Output(Bool())).suggestName("rom_busy")
//        busy := rom.module.io.rom_busy
//        busy
//      }
//      Some(rom_busy)
    }
    case None => None
  }
}
// DOC include end: ROM lazy trait

// DOC include start: ROM config fragment
class WithROM extends Config((site, here, up) => {
  case ROMKey => {
    Some(ROMParams(address = 0x10005000))
  }
})
// DOC include end: ROM config fragment