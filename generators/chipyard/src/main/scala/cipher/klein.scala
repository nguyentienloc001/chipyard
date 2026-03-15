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


// DOC include start: KLEIN params
case class KLEINParams(address: BigInt)
// DOC include end: KLEIN params


// DOC include start: KLEIN key
case object KLEINKey extends Field[Option[KLEINParams]](None)
// DOC include end: KLEIN key


class KLEINIO extends Bundle {
	val iclk 		    = Input(Clock())
	val ireset 		  = Input(Bool())
	val ics 		    = Input(Bool())
	val iwe         = Input(Bool())
  val iaddress    = Input(UInt(8.W))
  val iwrite_data = Input(UInt(32.W))
  val oread_data  = Output(UInt(32.W))
}

// declare register-map structure
object KLEINCtrlRegs {
  val trigger     = 0x00
  val data_a      = 0x04
  val data_b      = 0x08
  val data_c      = 0x0C
}


// DOC include start: KLEIN blackbox
class klein extends BlackBox with HasBlackBoxResource {
  val io = IO(new KLEINIO)
  addResource("/vsrc/KLEIN/klein.v")
  addResource("/vsrc/KLEIN/klein_core.v")
  addResource("/vsrc/KLEIN/klein_cipher.v")
  addResource("/vsrc/KLEIN/klein_decipher.v")
  addResource("/vsrc/KLEIN/klein_keyschedule.v")
  addResource("/vsrc/KLEIN/klein_mixcolumn.v")
  addResource("/vsrc/KLEIN/klein_sbox.v")
}
// DOC include end: KLEIN blackbox


// DOC include start: KLEIN router
class KLEINTL(params: KLEINParams, beatBytes: Int)(implicit p: Parameters) extends ClockSinkDomain(ClockSinkParameters())(p) {
  val device = new SimpleDevice("klein", Seq("deslab,klein"))
  val node = TLRegisterNode(
    address = Seq(AddressSet(params.address, 0xfff)),
    device = device,
    deviceKey =  "reg/control",
    beatBytes=beatBytes)

  override lazy val module = new KLEINImpl
  class KLEINImpl extends Impl {
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
      val impl = Module(new klein)
      val impl_io = impl.io

  		// mapping input
  		impl_io.iclk 					:= clock
  		impl_io.ireset 				:= reset.asBool || rst
    	impl_io.ics 					:= cs
    	impl_io.iwe 					:= we
    	impl_io.iaddress 			:= address
    	impl_io.iwrite_data 	:= write_data

  		// mapping output
  		read_data           	:= impl_io.oread_data

// DOC include start: KLEIN instance regmap
      node.regmap(
      	KLEINCtrlRegs.trigger -> Seq(
        	RegField(1, cs),
        	RegField(7),
        	RegField(1, we),
        	RegField(7),
        	RegField(1, rst)),
      	KLEINCtrlRegs.data_a -> Seq(RegField(8, address)),
      	KLEINCtrlRegs.data_b -> Seq(RegField(32, write_data)),
      	KLEINCtrlRegs.data_c -> Seq(RegField.r(32, read_data)))
// DOC include end: KLEIN instance regmap
    }
  }
}
// DOC include end: KLEIN router


// DOC include start: KLEIN lazy trait
trait CanHavePeripheryKLEIN { this: BaseSubsystem =>
  private val portName = "klein"

  private val pbus = locateTLBusWrapper(PBUS)

  p(KLEINKey) match {
    case Some(params) => {
      val klein = {
        val klein = LazyModule(new KLEINTL(params, pbus.beatBytes)(p))
        klein.clockNode := pbus.fixedClockNode
        pbus.coupleTo(portName) {
          klein.node := TLFragmenter(pbus.beatBytes, pbus.blockBytes) := _
        }
        klein
      }
    }
    case None => None
  }
}
// DOC include end: KLEIN lazy trait


// DOC include start: KLEIN config fragment
class WithKLEIN(address: BigInt) extends Config((site, here, up) => {
  case KLEINKey => {
    Some(KLEINParams(address = address))
  }
})
// DOC include end: KLEIN config fragment